// SPDX-License-Identifier: GPL-2.0+

#include <slab.h>
#include <types.h>
#include <export.h>
#include <virtio.h>
#include <virtio_ring.h>

#define to_vvq(_vq) container_of(_vq, struct vring_virtqueue, vq)

struct vring_virtqueue {
    struct virtqueue vq;

    /* Is this a packed ring? */
    bool packed_ring;

    /* Is DMA API used? */
    bool use_dma_api;

    /* Can we use weak barriers? */
    bool weak_barriers;

    /* Other side has made a mess, don't try any more. */
    bool broken;

    /* Host supports indirect buffers */
    bool indirect;

    /* Host publishes avail event idx */
    bool event;

    /* Head of free buffer list. */
    unsigned int free_head;
    /* Number we've added since last sync. */
    unsigned int num_added;

    /* Last used index we've seen. */
    u16 last_used_idx;

    struct {
        /* Actual memory layout for this queue. */
        struct vring vring;

        /* Last written value to avail->flags */
        u16 avail_flags_shadow;

        /*
         * Last written value to avail->idx in
         * guest byte order.
         */
        u16 avail_idx_shadow;

        /* Per-descriptor state. */
        struct vring_desc_state_split *desc_state;

        /* DMA address and size information */
        dma_addr_t queue_dma_addr;
        size_t queue_size_in_bytes;
    } split;

    /* How to notify other side. FIXME: commonalize hcalls! */
    bool (*notify)(struct virtqueue *vq);

    /* DMA, allocation, and size information */
    bool we_own_ring;
};

struct vring_desc_state_split {
    void *data;                     /* Data for callback. */
    struct vring_desc *indir_desc;  /* Indirect descriptor, if any. */
};

static void
vring_free_queue(struct virtio_device *vdev, size_t size, void *queue,
                 dma_addr_t dma_handle)
{
    free_pages_exact(queue, PAGE_ALIGN(size));
}

/* Manipulates transport-specific feature bits. */
void vring_transport_features(struct virtio_device *vdev)
{
    unsigned int i;

    for (i = VIRTIO_TRANSPORT_F_START; i < VIRTIO_TRANSPORT_F_END; i++) {
        switch (i) {
        case VIRTIO_RING_F_INDIRECT_DESC:
            break;
        case VIRTIO_RING_F_EVENT_IDX:
            break;
        case VIRTIO_F_VERSION_1:
            break;
        case VIRTIO_F_ACCESS_PLATFORM:
            break;
        case VIRTIO_F_RING_PACKED:
            break;
        case VIRTIO_F_ORDER_PLATFORM:
            break;
        default:
            /* We don't understand this bit. */
            __virtio_clear_bit(vdev, i);
        }
    }
}
EXPORT_SYMBOL(vring_transport_features);

static void *
vring_alloc_queue(struct virtio_device *vdev, size_t size,
                  dma_addr_t *dma_handle, gfp_t flag)
{
    void *queue = alloc_pages_exact(PAGE_ALIGN(size), flag);

    BUG_ON(virtio_has_feature(vdev, VIRTIO_F_ACCESS_PLATFORM));

    if (queue) {
        phys_addr_t phys_addr = virt_to_phys(queue);
        *dma_handle = (dma_addr_t)phys_addr;

        BUG_ON(*dma_handle != phys_addr);
    }
    return queue;
}

/* Only available for split ring */
struct virtqueue *
__vring_new_virtqueue(unsigned int index,
                      struct vring vring,
                      struct virtio_device *vdev,
                      bool weak_barriers,
                      bool context,
                      bool (*notify)(struct virtqueue *),
                      void (*callback)(struct virtqueue *),
                      const char *name)
{
    unsigned int i;
    struct vring_virtqueue *vq;

    if (virtio_has_feature(vdev, VIRTIO_F_RING_PACKED))
        panic("unsupport ring packed!");

    vq = kmalloc(sizeof(*vq), GFP_KERNEL);
    if (!vq)
        return NULL;

    vq->packed_ring = false;
    vq->vq.callback = callback;
    vq->vq.vdev = vdev;
    vq->vq.name = name;
    vq->vq.num_free = vring.num;
    vq->vq.index = index;
    vq->we_own_ring = false;
    vq->notify = notify;
    vq->weak_barriers = weak_barriers;
    vq->broken = false;
    vq->last_used_idx = 0;
    vq->num_added = 0;
    vq->use_dma_api = false;
    list_add_tail(&vq->vq.list, &vdev->vqs);

    vq->indirect = virtio_has_feature(vdev, VIRTIO_RING_F_INDIRECT_DESC) && !context;
    vq->event = virtio_has_feature(vdev, VIRTIO_RING_F_EVENT_IDX);

    if (virtio_has_feature(vdev, VIRTIO_F_ORDER_PLATFORM))
        vq->weak_barriers = false;

    vq->split.queue_dma_addr = 0;
    vq->split.queue_size_in_bytes = 0;

    vq->split.vring = vring;
    vq->split.avail_flags_shadow = 0;
    vq->split.avail_idx_shadow = 0;

    /* No callback?  Tell other side not to bother us. */
    if (!callback) {
        vq->split.avail_flags_shadow |= VRING_AVAIL_F_NO_INTERRUPT;
        if (!vq->event)
            vq->split.vring.avail->flags = (u16)(vq->split.avail_flags_shadow);
    }

    vq->split.desc_state =
        kmalloc_array(vring.num, sizeof(struct vring_desc_state_split),
                      GFP_KERNEL);
    if (!vq->split.desc_state) {
        kfree(vq);
        return NULL;
    }

    /* Put everything in free lists. */
    vq->free_head = 0;
    for (i = 0; i < vring.num-1; i++)
        vq->split.vring.desc[i].next = (u16)(i + 1);
    memset(vq->split.desc_state, 0,
           vring.num * sizeof(struct vring_desc_state_split));

    return &vq->vq;
}

static struct virtqueue *
vring_create_virtqueue_split(unsigned int index,
                             unsigned int num,
                             unsigned int vring_align,
                             struct virtio_device *vdev,
                             bool weak_barriers,
                             bool may_reduce_num,
                             bool context,
                             bool (*notify)(struct virtqueue *),
                             void (*callback)(struct virtqueue *),
                             const char *name)
{
    dma_addr_t dma_addr;
    size_t queue_size_in_bytes;
    struct vring vring;
    struct virtqueue *vq;
    void *queue = NULL;

    /* We assume num is a power of 2. */
    if (num & (num - 1)) {
        panic("Bad virtqueue length %u\n", num);
        return NULL;
    }

    /* TODO: allocate each queue chunk individually */
    for (; num && vring_size(num, vring_align) > PAGE_SIZE; num /= 2) {
        queue = vring_alloc_queue(vdev, vring_size(num, vring_align),
                                  &dma_addr,
                                  GFP_KERNEL|__GFP_NOWARN|__GFP_ZERO);
        if (queue)
            break;
        if (!may_reduce_num)
            return NULL;
    }

    if (!num)
        return NULL;

    if (!queue) {
        /* Try to get a single page. You are my only hope! */
        queue = vring_alloc_queue(vdev, vring_size(num, vring_align),
                                  &dma_addr, GFP_KERNEL|__GFP_ZERO);
    }
    if (!queue)
        return NULL;

    queue_size_in_bytes = vring_size(num, vring_align);
    vring_init(&vring, num, queue, vring_align);

    vq = __vring_new_virtqueue(index, vring, vdev, weak_barriers, context,
                               notify, callback, name);
    if (!vq) {
        vring_free_queue(vdev, queue_size_in_bytes, queue,
                 dma_addr);
        return NULL;
    }

    to_vvq(vq)->split.queue_dma_addr = dma_addr;
    to_vvq(vq)->split.queue_size_in_bytes = queue_size_in_bytes;
    to_vvq(vq)->we_own_ring = true;

    return vq;
}

struct virtqueue *
vring_create_virtqueue(unsigned int index,
                       unsigned int num,
                       unsigned int vring_align,
                       struct virtio_device *vdev,
                       bool weak_barriers,
                       bool may_reduce_num,
                       bool context,
                       bool (*notify)(struct virtqueue *),
                       void (*callback)(struct virtqueue *),
                       const char *name)
{
    return vring_create_virtqueue_split(index, num, vring_align,
                                        vdev, weak_barriers,
                                        may_reduce_num, context, notify,
                                        callback, name);
}
EXPORT_SYMBOL(vring_create_virtqueue);

unsigned int
virtqueue_get_vring_size(struct virtqueue *_vq)
{
    return to_vvq(_vq)->split.vring.num;
}
EXPORT_SYMBOL(virtqueue_get_vring_size);

dma_addr_t
virtqueue_get_desc_addr(struct virtqueue *_vq)
{
    struct vring_virtqueue *vq = to_vvq(_vq);

    BUG_ON(!vq->we_own_ring);
    return vq->split.queue_dma_addr;
}
EXPORT_SYMBOL(virtqueue_get_desc_addr);
