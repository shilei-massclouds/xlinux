// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <slab.h>
#include <block.h>
#include <errno.h>
#include <genhd.h>
#include <sysfs.h>
#include <blkdev.h>
#include <blk-mq.h>
#include <export.h>
#include <printk.h>
#include <virtio.h>
#include <virtio_blk.h>
#include <virtio_ring.h>

#define PART_BITS 4
#define VQ_NAME_LEN 16

static int major;
static u32 _vd_index_ida;

struct virtio_blk_vq {
    struct virtqueue *vq;
    char name[VQ_NAME_LEN];
};

struct virtio_blk {
    struct virtio_device *vdev;

    /* The disk structure for the kernel. */
    struct gendisk *disk;

    /* Block layer tags. */
    struct blk_mq_tag_set tag_set;

    /* What host tells us, plus 2 for header & tailer. */
    unsigned int sg_elems;

    /* Ida index - used to track minor number allocations. */
    int index;

    /* num of vqs */
    int num_vqs;
    struct virtio_blk_vq *vqs;
};

static const struct virtio_device_id id_table[] = {
    { VIRTIO_ID_BLOCK, VIRTIO_DEV_ANY_ID },
    { 0 },
};

static unsigned int features_legacy[] = {
    VIRTIO_BLK_F_SEG_MAX, VIRTIO_BLK_F_SIZE_MAX, VIRTIO_BLK_F_GEOMETRY,
    VIRTIO_BLK_F_RO, VIRTIO_BLK_F_BLK_SIZE,
    VIRTIO_BLK_F_FLUSH, VIRTIO_BLK_F_TOPOLOGY, VIRTIO_BLK_F_CONFIG_WCE,
    VIRTIO_BLK_F_MQ, VIRTIO_BLK_F_DISCARD, VIRTIO_BLK_F_WRITE_ZEROES,
};

static unsigned int features[] = {
    VIRTIO_BLK_F_SEG_MAX, VIRTIO_BLK_F_SIZE_MAX, VIRTIO_BLK_F_GEOMETRY,
    VIRTIO_BLK_F_RO, VIRTIO_BLK_F_BLK_SIZE,
    VIRTIO_BLK_F_FLUSH, VIRTIO_BLK_F_TOPOLOGY, VIRTIO_BLK_F_CONFIG_WCE,
    VIRTIO_BLK_F_MQ, VIRTIO_BLK_F_DISCARD, VIRTIO_BLK_F_WRITE_ZEROES,
};

static void
virtblk_done(struct virtqueue *vq)
{
    panic("Implement it!");
}

static int
init_vq(struct virtio_blk *vblk)
{
    int i;
    int err;
    const char **names;
    vq_callback_t **callbacks;
    unsigned short num_vqs;
    struct virtqueue **vqs;
    struct virtio_device *vdev = vblk->vdev;
    struct irq_affinity desc = { 0, };

    err = virtio_cread_feature(vdev, VIRTIO_BLK_F_MQ,
                               struct virtio_blk_config, num_queues,
                               &num_vqs);
    if (err)
        num_vqs = 1;

    vblk->vqs = kmalloc_array(num_vqs, sizeof(*vblk->vqs), GFP_KERNEL);
    if (!vblk->vqs)
        return -ENOMEM;

    names = kmalloc_array(num_vqs, sizeof(*names), GFP_KERNEL);
    callbacks = kmalloc_array(num_vqs, sizeof(*callbacks), GFP_KERNEL);
    vqs = kmalloc_array(num_vqs, sizeof(*vqs), GFP_KERNEL);
    if (!names || !callbacks || !vqs) {
        err = -ENOMEM;
        goto out;
    }

    for (i = 0; i < num_vqs; i++) {
        callbacks[i] = virtblk_done;
        snprintf(vblk->vqs[i].name, VQ_NAME_LEN, "req.%d", i);
        names[i] = vblk->vqs[i].name;
    }

    /* Discover virtqueues and write information to configuration. */
    err = virtio_find_vqs(vdev, num_vqs, vqs, callbacks, names, &desc);
    if (err)
        goto out;

    for (i = 0; i < num_vqs; i++) {
        vblk->vqs[i].vq = vqs[i];
    }
    vblk->num_vqs = num_vqs;

out:
    kfree(vqs);
    kfree(callbacks);
    kfree(names);
    if (err)
        kfree(vblk->vqs);
    return err;
}

/*
 * Legacy naming scheme used for virtio devices.
 * We are stuck with it for virtio blk but don't ever use it
 * for any new driver.
 */
static int
virtblk_name_format(char *prefix, int index, char *buf, int buflen)
{
    char *p;
    int unit;
    const int base = 'z' - 'a' + 1;
    char *end = buf + buflen;
    char *begin = buf + strlen(prefix);

    p = end - 1;
    *p = '\0';
    unit = base;
    do {
        if (p == begin)
            return -EINVAL;
        *--p = 'a' + (index % unit);
        index = (index / unit) - 1;
    } while (index >= 0);

    memmove(begin, p, end - p);
    memcpy(buf, prefix, strlen(prefix));
    return 0;
}

static void
virtblk_get(struct virtio_blk *vblk)
{
}

static int
virtblk_open(struct block_device *bd, fmode_t mode)
{
    int ret = 0;
    struct virtio_blk *vblk = bd->bd_disk->private_data;

    if (vblk->vdev)
        virtblk_get(vblk);
    else
        ret = -ENXIO;

    return ret;
}

static const struct block_device_operations virtblk_fops = {
    .open = virtblk_open,
    //.getgeo = virtblk_getgeo,
};

static int
index_to_minor(int index)
{
    return index << PART_BITS;
}

static int
virtblk_get_cache_mode(struct virtio_device *vdev)
{
    int err;
    u8 writeback;

    err = virtio_cread_feature(vdev, VIRTIO_BLK_F_CONFIG_WCE,
                               struct virtio_blk_config, wce,
                               &writeback);

    /*
     * If WCE is not configurable and flush is not available,
     * assume no writeback cache is in use.
     */
    if (err)
        writeback = virtio_has_feature(vdev, VIRTIO_BLK_F_FLUSH);

    return writeback;
}

void
blk_queue_write_cache(struct request_queue *q, bool wc, bool fua)
{
    /* Todo: */
}
EXPORT_SYMBOL(blk_queue_write_cache);

static void
virtblk_update_cache_mode(struct virtio_device *vdev)
{
    struct virtio_blk *vblk = vdev->priv;
    u8 writeback = virtblk_get_cache_mode(vdev);

    blk_queue_write_cache(vblk->disk->queue, writeback, false);
    //revalidate_disk(vblk->disk);
}

void
set_capacity_revalidate_and_notify(struct gendisk *disk,
                                   sector_t size,
                                   bool revalidate)
{
    set_capacity(disk, size);
}
EXPORT_SYMBOL(set_capacity_revalidate_and_notify);

static void
virtblk_update_capacity(struct virtio_blk *vblk, bool resize)
{
    struct virtio_device *vdev = vblk->vdev;
    struct request_queue *q = vblk->disk->queue;
    char cap_str_2[10], cap_str_10[10];
    unsigned long long nblocks;
    u64 capacity;

    /* Host must always specify the capacity. */
    virtio_cread(vdev, struct virtio_blk_config, capacity, &capacity);

    /* If capacity is too big, truncate with warning. */
    if ((sector_t)capacity != capacity) {
        panic("Capacity %lu too large: truncating\n",
              (unsigned long long)capacity);
        capacity = (sector_t)-1;
    }

    nblocks = DIV_ROUND_UP_ULL(capacity, queue_logical_block_size(q) >> 9);

    printk("[%s] %s%lu %d-byte logical blocks\n",
           vblk->disk->disk_name,
           resize ? "new size: " : "",
           nblocks,
           queue_logical_block_size(q));

    set_capacity_revalidate_and_notify(vblk->disk, capacity, true);
}

static struct attribute *virtblk_attrs[] = {
    NULL,   /* &dev_attr_serial.attr, */
    NULL,   /* &dev_attr_cache_type.attr, */
    NULL,
};

static umode_t
virtblk_attrs_are_visible(struct kobject *kobj,
                          struct attribute *a,
                          int n)
{
    /*
    struct device *dev = container_of(kobj, struct device, kobj);
    struct gendisk *disk = dev_to_disk(dev);
    struct virtio_blk *vblk = disk->private_data;
    struct virtio_device *vdev = vblk->vdev;

    if (a == &dev_attr_cache_type.attr &&
        !virtio_has_feature(vdev, VIRTIO_BLK_F_CONFIG_WCE))
        return S_IRUGO;

    return a->mode;
    */
}

static const struct attribute_group virtblk_attr_group = {
    .attrs = virtblk_attrs,
    .is_visible = virtblk_attrs_are_visible,
};

static const struct attribute_group *virtblk_attr_groups[] = {
    &virtblk_attr_group,
    NULL,
};

static unsigned int virtblk_queue_depth;

static int
virtblk_probe(struct virtio_device *vdev)
{
    int err;
    int index;
    u32 sg_elems;
    u32 blk_size;
    struct virtio_blk *vblk;
    struct request_queue *q;

    if (!vdev->config->get) {
        panic("%s failure: config access disabled", __func__);
        return -EINVAL;
    }

    index = _vd_index_ida++;

    /* We need to know how many segments before we allocate. */
    err = virtio_cread_feature(vdev, VIRTIO_BLK_F_SEG_MAX,
                               struct virtio_blk_config, seg_max,
                               &sg_elems);

    /* We need at least one SG element, whatever they say. */
    if (err || !sg_elems)
        sg_elems = 1;

    /* We need an extra sg elements at head and tail. */
    sg_elems += 2;
    vdev->priv = vblk = kmalloc(sizeof(*vblk), GFP_KERNEL);
    if (!vblk)
        panic("NO memory!");

    vblk->vdev = vdev;
    vblk->sg_elems = sg_elems;

    err = init_vq(vblk);
    if (err)
        panic("bad vq!");

    /* FIXME: How many partitions?  How long is a piece of string? */
    vblk->disk = alloc_disk(1 << PART_BITS);
    if (!vblk->disk)
        panic("NO memory!");

    BUG_ON(!virtio_has_feature(vdev, VIRTIO_RING_F_INDIRECT_DESC));

    /* Default queue sizing is to fill the ring. */
    if (!virtblk_queue_depth) {
        virtblk_queue_depth = vblk->vqs[0].vq->num_free;
    }

    memset(&vblk->tag_set, 0, sizeof(vblk->tag_set));
    vblk->tag_set.nr_hw_queues = vblk->num_vqs;

    /*
    err = blk_mq_alloc_tag_set(&vblk->tag_set);
    if (err)
        panic("alloc tag set!");
        */

    q = blk_mq_init_queue(&vblk->tag_set);
    if (IS_ERR(q)) {
        err = -ENOMEM;
        panic("out of memory!");
    }
    vblk->disk->queue = q;

    q->queuedata = vblk;

    virtblk_name_format("vd", index, vblk->disk->disk_name, DISK_NAME_LEN);

    vblk->disk->major = major;
    vblk->disk->first_minor = index_to_minor(index);
    vblk->disk->private_data = vblk;
    vblk->disk->fops = &virtblk_fops;
    vblk->disk->flags |= GENHD_FL_EXT_DEVT;
    vblk->index = index;

    /* configure queue flush support */
    virtblk_update_cache_mode(vdev);

    printk("%s: %s sg_elems(%u) major(%x)\n",
           __func__, vblk->disk->disk_name,
           vblk->sg_elems, major);

    /* Host can optionally specify the block size of the device */
    err = virtio_cread_feature(vdev, VIRTIO_BLK_F_BLK_SIZE,
                               struct virtio_blk_config, blk_size,
                               &blk_size);
    if (!err)
        blk_queue_logical_block_size(q, blk_size);
    else
        blk_size = queue_logical_block_size(q);

    virtblk_update_capacity(vblk, false);

    virtio_device_ready(vdev);

    device_add_disk(&vdev->dev, vblk->disk, virtblk_attr_groups);
    return 0;
}

static struct virtio_driver virtio_blk = {
    .driver.name    = "virtio_blk",
    .id_table       = id_table,
    .probe          = virtblk_probe,

    .feature_table              = features,
    .feature_table_size         = ARRAY_SIZE(features),
    .feature_table_legacy       = features_legacy,
    .feature_table_size_legacy  = ARRAY_SIZE(features_legacy),
};

static int
init_module(void)
{
    printk("module[virtio_blk]: init begin ...\n");

    major = register_blkdev(0, "virtblk");
    if (major < 0)
        panic("cannot register blk device!");

    if (register_virtio_driver(&virtio_blk))
        panic("cannot register virtio driver!");

    printk("module[virtio_blk]: init end!\n");
    return 0;
}
