// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <slab.h>
#include <errno.h>
#include <printk.h>
#include <virtio.h>
#include <virtio_blk.h>

#define VQ_NAME_LEN 16

struct virtio_blk_vq {
    struct virtqueue *vq;
    char name[VQ_NAME_LEN];
};

struct virtio_blk {
    struct virtio_device *vdev;

    /* What host tells us, plus 2 for header & tailer. */
    unsigned int sg_elems;

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

static int
virtblk_probe(struct virtio_device *vdev)
{
    int err;
    int index;
    u32 sg_elems;
    struct virtio_blk *vblk;

    if (!vdev->config->get) {
        panic("%s failure: config access disabled", __func__);
        return -EINVAL;
    }

    index = _virtio_index++;

    /* We need to know how many segments before we allocate. */
    err = virtio_cread_feature(vdev, VIRTIO_BLK_F_SEG_MAX,
                               struct virtio_blk_config, seg_max,
                               &sg_elems);

    printk("%s: sg_elems(%u)\n", __func__, sg_elems);

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

    panic("%s:", __func__);
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

    if (register_virtio_driver(&virtio_blk))
        panic("cannot register!");

    printk("module[virtio_blk]: init end!\n");
    return 0;
}
