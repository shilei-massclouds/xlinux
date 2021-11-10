// SPDX-License-Identifier: GPL-2.0+

#include <mmio.h>
#include <errno.h>
#include <devres.h>
#include <export.h>
#include <virtio.h>
#include <platform.h>
#include <virtio_mmio.h>

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

static int virtio_mmio_probe(struct platform_device *pdev)
{
    unsigned long magic;
    struct virtio_mmio_device *vm_dev;

    vm_dev = devm_kzalloc(&pdev->dev, sizeof(*vm_dev), GFP_KERNEL);
    if (!vm_dev)
        return -ENOMEM;

    vm_dev->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(vm_dev->base))
        return PTR_ERR(vm_dev->base);

    /* Check magic value */
    magic = readl(vm_dev->base + VIRTIO_MMIO_MAGIC_VALUE);
    if (magic != ('v' | 'i' << 8 | 'r' << 16 | 't' << 24)) {
        panic("Wrong magic value 0x%lx!\n", magic);
        return -ENODEV;
    }

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

int virtio_mmio_init(void)
{
    return platform_driver_register(&virtio_mmio_driver);
}
EXPORT_SYMBOL(virtio_mmio_init);
