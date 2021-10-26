// SPDX-License-Identifier: GPL-2.0+

#include <export.h>
#include <platform.h>

/* Platform driver */

static const struct of_device_id virtio_mmio_match[] = {
    { .compatible = "virtio,mmio", },
    {},
};

static int virtio_mmio_probe(struct platform_device *pdev)
{
    panic("bad probe!");
    return -1;
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
