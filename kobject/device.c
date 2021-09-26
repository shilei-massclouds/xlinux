// SPDX-License-Identifier: GPL-2.0
#include <printk.h>
#include <export.h>
#include <device.h>
#include <errno.h>
#include <memblock.h>

static int
device_private_init(struct device *dev)
{
    dev->p = memblock_alloc(sizeof(*dev->p), 8);
    if (!dev->p)
        return -ENOMEM;
    dev->p->device = dev;
    return 0;
}

int device_add(struct device *dev)
{
    int error = -EINVAL;

    printk("device: '%s': %s\n", dev_name(dev), __func__);
    
    if (!dev->p) {
        error = device_private_init(dev);
        if (error)
            goto done;
    }
    
    error = bus_add_device(dev);
    if (error)
        goto done;

    return 0;

 done:
    return error;
}
EXPORT_SYMBOL(device_add);

/**
 * put_device - decrement reference count.
 * @dev: device in question.
 */
void put_device(struct device *dev)
{
    /* Todo: add kobject_put */
}
EXPORT_SYMBOL(put_device);
