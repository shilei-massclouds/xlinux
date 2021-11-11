// SPDX-License-Identifier: GPL-2.0+

#include <export.h>
#include <printk.h>
#include <virtio.h>

static int
virtio_dev_probe(struct device *_d)
{
    struct virtio_device *dev = dev_to_virtio(_d);
    struct virtio_driver *drv = drv_to_virtio(dev->dev.driver);

    panic("%s: \n", __func__);
}

static inline int
virtio_id_match(const struct virtio_device *dev,
                const struct virtio_device_id *id)
{
    if (id->device != dev->id.device && id->device != VIRTIO_DEV_ANY_ID)
        return 0;

    return id->vendor == VIRTIO_DEV_ANY_ID || id->vendor == dev->id.vendor;
}

static int
virtio_dev_match(struct device *_dv, struct device_driver *_dr)
{
    unsigned int i;
    struct virtio_device *dev = dev_to_virtio(_dv);
    const struct virtio_device_id *ids;

    ids = drv_to_virtio(_dr)->id_table;
    for (i = 0; ids[i].device; i++)
        if (virtio_id_match(dev, &ids[i]))
            return 1;
    return 0;
}

static struct bus_type virtio_bus = {
    .name  = "virtio",
    .match = virtio_dev_match,
    .probe = virtio_dev_probe,
};

static int _virtio_index;

void
virtio_add_status(struct virtio_device *dev, unsigned int status)
{
    dev->config->set_status(dev, dev->config->get_status(dev) | status);
}
EXPORT_SYMBOL(virtio_add_status);

/**
 * register_virtio_device - register virtio device
 * @dev: virtio device to be registered
 *
 * On error, the caller must call put_device
 * on &@dev->dev (and not kfree),
 * as another code path may have obtained a reference to @dev.
 *
 * Returns: 0 on suceess, -error on failure
 */
int
register_virtio_device(struct virtio_device *dev)
{
    int err;

    dev->dev.bus = &virtio_bus;
    device_initialize(&dev->dev);

    dev->index = _virtio_index++;
    dev_set_name(&dev->dev, "virtio%u", dev->index);

    dev->config_enabled = false;
    dev->config_change_pending = false;

    /* We always start by resetting the device, in case a previous
     * driver messed it up.  This also tests that code path a little. */
    dev->config->reset(dev);

    /* Acknowledge that we've seen the device. */
    virtio_add_status(dev, VIRTIO_CONFIG_S_ACKNOWLEDGE);

    INIT_LIST_HEAD(&dev->vqs);

    /*
     * device_add() causes the bus infrastructure to look for a matching
     * driver.
     */
    err = device_add(&dev->dev);
    if (err) {
        virtio_add_status(dev, VIRTIO_CONFIG_S_FAILED);
        panic("can not add device!");
    }
    return err;
}
EXPORT_SYMBOL(register_virtio_device);

static int
virtio_init(void)
{
    if (bus_register(&virtio_bus) != 0)
        panic("virtio bus registration failed");
    return 0;
}

static int
init_module(void)
{
    printk("module[virtio]: init begin ...\n");
    virtio_init();
    printk("module[virtio]: init end!\n");
    return 0;
}
