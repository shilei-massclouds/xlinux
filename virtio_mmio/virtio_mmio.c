// SPDX-License-Identifier: GPL-2.0+

#include <mmio.h>
#include <errno.h>
#include <devres.h>
#include <export.h>
#include <virtio.h>
#include <platform.h>
#include <virtio_mmio.h>
#include <virtio_config.h>

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

static const struct virtio_config_ops virtio_mmio_config_ops = {
    /*
    .get        = vm_get,
    .set        = vm_set,
    .generation = vm_generation,
    .get_status = vm_get_status,
    .set_status = vm_set_status,
    .reset      = vm_reset,
    .find_vqs   = vm_find_vqs,
    .del_vqs    = vm_del_vqs,
    .get_features   = vm_get_features,
    .finalize_features = vm_finalize_features,
    .bus_name   = vm_bus_name,
    */
};

static int
virtio_mmio_probe(struct platform_device *pdev)
{
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

    printk("%s: %s ok!\n", __func__, pdev->name);
    return -ENODEV;
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
