// SPDX-License-Identifier: GPL-2.0
#include <acgcc.h>
#include <printk.h>
#include <export.h>
#include <device.h>
#include <errno.h>
#include <slab.h>

static int
device_private_init(struct device *dev)
{
    dev->p = kzalloc(sizeof(*dev->p), GFP_KERNEL);
    if (!dev->p)
        return -ENOMEM;
    dev->p->device = dev;
    return 0;
}

int device_add(struct device *dev)
{
    int error = -EINVAL;

    pr_debug("device: '%s': %s\n", dev_name(dev), __func__);

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

/**
 * dev_set_name - set a device name
 * @dev: device
 * @fmt: format string for the device's name
 */
int
dev_set_name(struct device *dev, const char *fmt, ...)
{
    va_list vargs;
    int err;

    va_start(vargs, fmt);
    err = kobject_set_name_vargs(&dev->kobj, fmt, vargs);
    va_end(vargs);
    return err;
}
EXPORT_SYMBOL(dev_set_name);
