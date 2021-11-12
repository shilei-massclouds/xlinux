// SPDX-License-Identifier: GPL-2.0+

#include <mmio.h>
#include <errno.h>
#include <devres.h>
#include <export.h>
#include <virtio.h>
#include <platform.h>
#include <virtio_mmio.h>
#include <virtio_config.h>
#include <virtio_ring.h>

#define to_virtio_mmio_device(_plat_dev) \
    container_of(_plat_dev, struct virtio_mmio_device, vdev)

struct virtio_mmio_device {
    struct virtio_device vdev;
    struct platform_device *pdev;

    void *base;
    unsigned long version;

    /* a list of queues so we can dispatch IRQs */
    struct list_head virtqueues;
};

/* Platform driver */

static const struct of_device_id virtio_mmio_match[] = {
    { .compatible = "virtio,mmio", },
    {},
};

static u8
vm_get_status(struct virtio_device *vdev)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);

    return readl(vm_dev->base + VIRTIO_MMIO_STATUS) & 0xff;
}

static void
vm_set_status(struct virtio_device *vdev, u8 status)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);

    /* We should never be setting status to 0. */
    BUG_ON(status == 0);

    writel(status, vm_dev->base + VIRTIO_MMIO_STATUS);
}

static void
vm_reset(struct virtio_device *vdev)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);

    /* 0 status means a reset. */
    writel(0, vm_dev->base + VIRTIO_MMIO_STATUS);
}

static u64
vm_get_features(struct virtio_device *vdev)
{
    u64 features;
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);

    writel(1, vm_dev->base + VIRTIO_MMIO_DEVICE_FEATURES_SEL);
    features = readl(vm_dev->base + VIRTIO_MMIO_DEVICE_FEATURES);
    features <<= 32;

    writel(0, vm_dev->base + VIRTIO_MMIO_DEVICE_FEATURES_SEL);
    features |= readl(vm_dev->base + VIRTIO_MMIO_DEVICE_FEATURES);

    return features;
}

static int
vm_finalize_features(struct virtio_device *vdev)
{
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);

    /* Give virtio_ring a chance to accept features. */
    vring_transport_features(vdev);

    /* Make sure there is are no mixed devices */
    if (vm_dev->version == 2 &&
            !__virtio_test_bit(vdev, VIRTIO_F_VERSION_1)) {
        panic("New virtio-mmio devices (version 2) must provide VIRTIO_F_VERSION_1 feature!\n");
        return -EINVAL;
    }

    writel(1, vm_dev->base + VIRTIO_MMIO_DRIVER_FEATURES_SEL);
    writel((u32)(vdev->features >> 32),
           vm_dev->base + VIRTIO_MMIO_DRIVER_FEATURES);

    writel(0, vm_dev->base + VIRTIO_MMIO_DRIVER_FEATURES_SEL);
    writel((u32)vdev->features,
           vm_dev->base + VIRTIO_MMIO_DRIVER_FEATURES);

    return 0;
}

static void
vm_get(struct virtio_device *vdev, unsigned offset, void *buf, unsigned len)
{
    u8 b;
    u16 w;
    u32 l;
    struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
    void *base = vm_dev->base + VIRTIO_MMIO_CONFIG;

    if (vm_dev->version == 1) {
        u8 *ptr = buf;
        int i;

        for (i = 0; i < len; i++)
            ptr[i] = readb(base + offset + i);
        return;
    }

    switch (len) {
    case 1:
        b = readb(base + offset);
        memcpy(buf, &b, sizeof b);
        break;
    case 2:
        w = readw(base + offset);
        memcpy(buf, &w, sizeof w);
        break;
    case 4:
        l = readl(base + offset);
        memcpy(buf, &l, sizeof l);
        break;
    case 8:
        l = readl(base + offset);
        memcpy(buf, &l, sizeof l);
        l = readl(base + offset + sizeof l);
        memcpy(buf + sizeof l, &l, sizeof l);
        break;
    default:
        BUG();
    }
}

static const struct virtio_config_ops virtio_mmio_config_ops = {
    .get        = vm_get,
    /*
    .set        = vm_set,
    .generation = vm_generation,
    */
    .reset          = vm_reset,
    .get_status     = vm_get_status,
    .set_status     = vm_set_status,
    .get_features   = vm_get_features,
    .finalize_features = vm_finalize_features,
    /*
    .find_vqs   = vm_find_vqs,
    .del_vqs    = vm_del_vqs,
    .bus_name   = vm_bus_name,
    */
};

static int
virtio_mmio_probe(struct platform_device *pdev)
{
    int rc;
    unsigned long magic;
    struct virtio_mmio_device *vm_dev;

    vm_dev = devm_kzalloc(&pdev->dev, sizeof(*vm_dev), GFP_KERNEL);
    if (!vm_dev)
        return -ENOMEM;

    vm_dev->vdev.dev.parent = &pdev->dev;
    vm_dev->vdev.config = &virtio_mmio_config_ops;
    vm_dev->pdev = pdev;
    INIT_LIST_HEAD(&vm_dev->virtqueues);

    vm_dev->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(vm_dev->base))
        return PTR_ERR(vm_dev->base);

    /* Check magic value */
    magic = readl(vm_dev->base + VIRTIO_MMIO_MAGIC_VALUE);
    if (magic != ('v' | 'i' << 8 | 'r' << 16 | 't' << 24)) {
        panic("Wrong magic value 0x%lx!\n", magic);
        return -ENODEV;
    }

    /* Check device version */
    vm_dev->version = readl(vm_dev->base + VIRTIO_MMIO_VERSION);
    if (vm_dev->version < 1 || vm_dev->version > 2) {
        panic("Version %ld not supported!\n", vm_dev->version);
        return -ENXIO;
    }
    BUG_ON(vm_dev->version != 1);

    vm_dev->vdev.id.device = readl(vm_dev->base + VIRTIO_MMIO_DEVICE_ID);
    if (vm_dev->vdev.id.device == 0) {
        /*
         * virtio-mmio device with an ID 0 is a (dummy) placeholder
         * with no function. End probing now with no error reported.
         */
        return -ENODEV;
    }
    vm_dev->vdev.id.vendor = readl(vm_dev->base + VIRTIO_MMIO_VENDOR_ID);

    writel(PAGE_SIZE, vm_dev->base + VIRTIO_MMIO_GUEST_PAGE_SIZE);

    platform_set_drvdata(pdev, vm_dev);

    rc = register_virtio_device(&vm_dev->vdev);
    if (rc)
        put_device(&vm_dev->vdev.dev);

    printk("%s: %s ok!\n", __func__, pdev->name);
    return rc;
}

static struct platform_driver virtio_mmio_driver = {
    .probe      = virtio_mmio_probe,
    .driver     = {
        .name   = "virtio-mmio",
        .of_match_table = virtio_mmio_match,
    },
};

int
virtio_mmio_init(void)
{
    return platform_driver_register(&virtio_mmio_driver);
}
EXPORT_SYMBOL(virtio_mmio_init);

static int
init_module(void)
{
    printk("module[virtio_mmio]: init begin ...\n");
    virtio_mmio_init();
    printk("module[virtio_mmio]: init end!\n");
    return 0;
}
