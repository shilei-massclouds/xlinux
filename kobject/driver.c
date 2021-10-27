// SPDX-License-Identifier: GPL-2.0

#include <klist.h>
#include <errno.h>
#include <export.h>
#include <driver.h>

struct device_driver *
driver_find(const char *name, struct bus_type *bus)
{
    struct driver_private *priv;
    struct kobject *k = kset_find_obj(bus->p->drivers_kset, name);

    if (k) {
        kobject_put(k);
        priv = to_driver(k);
        return priv->driver;
    }
    return NULL;
}
EXPORT_SYMBOL(driver_find);

int
driver_register(struct device_driver *drv)
{
    int ret;
    struct device_driver *other;

    if (!drv->bus->p) {
        panic("Driver '%s' was unable to register with bus_type '%s' "
              "because the bus was not initialized.\n",
               drv->name, drv->bus->name);
        return -EINVAL;
    }

    other = driver_find(drv->name, drv->bus);
    if (other) {
        panic("Error: Driver '%s' is already registered, aborting...",
              drv->name);
        return -EBUSY;
    }

    ret = bus_add_driver(drv);
    if (ret)
        return ret;
    /*
    ret = driver_add_groups(drv, drv->groups);
    if (ret) {
        bus_remove_driver(drv);
        return ret;
    }
    */

    return ret;
}
EXPORT_SYMBOL(driver_register);

static struct device *
next_device(struct klist_iter *i)
{
    struct device_private *dev_prv;
    struct device *dev = NULL;
    struct klist_node *n = klist_next(i);

    if (n) {
        dev_prv = to_device_private_bus(n);
        dev = dev_prv->device;
    }
    return dev;
}

int
bus_for_each_dev(struct bus_type *bus,
                 struct device *start,
                 void *data,
                 int (*fn)(struct device *, void *))
{
    struct klist_iter i;
    struct device *dev;
    int error = 0;

    if (!bus || !bus->p)
        return -EINVAL;

    klist_iter_init_node(&bus->p->klist_devices,
                         &i,
                         (start ? &start->p->knode_bus : NULL));
    while (!error && (dev = next_device(&i)))
        error = fn(dev, data);
    klist_iter_exit(&i);
    return error;
}
EXPORT_SYMBOL(bus_for_each_dev);

bool
device_is_bound(struct device *dev)
{
    return dev->p && klist_node_attached(&dev->p->knode_driver);
}

static void
driver_bound(struct device *dev)
{
    if (device_is_bound(dev)) {
        panic("%s: device %s already bound",
              __func__, kobject_name(&dev->kobj));
        return;
    }

    klist_add_tail(&dev->p->knode_driver,
                   &dev->driver->p->klist_devices);
}

static int
really_probe(struct device *dev, struct device_driver *drv)
{
    int ret;

    dev->driver = drv;

    if (dev->bus->probe) {
        ret = dev->bus->probe(dev);
        if (ret)
            return 0;
    } else if (drv->probe) {
        ret = drv->probe(dev);
        if (ret)
            return 0;
    }

    driver_bound(dev);
    return 0;
}

int
driver_probe_device(struct device_driver *drv, struct device *dev)
{
    printk("bus: '%s': %s: matched device %s with driver %s\n",
           drv->bus->name, __func__, dev_name(dev), drv->name);

    return really_probe(dev, drv);
}

int
device_driver_attach(struct device_driver *drv, struct device *dev)
{
    int ret = 0;

    /*
     * If device has been removed or someone has already successfully
     * bound a driver before us just skip the driver probe call.
     */
    if (!dev->driver)
        ret = driver_probe_device(drv, dev);

    return ret;
}

static int
__driver_attach(struct device *dev, void *data)
{
    int ret;
    struct device_driver *drv = data;

    ret = driver_match_device(drv, dev);
    if (ret == 0) {
        /* no match */
        return 0;
    } else if (ret == -EPROBE_DEFER) {
        panic("Device match requests probe deferral");
    } else if (ret < 0) {
        panic("Bus failed to match device: %d", ret);
    }

    device_driver_attach(drv, dev);
    return 0;
}

int
driver_attach(struct device_driver *drv)
{
    return bus_for_each_dev(drv->bus, NULL, drv, __driver_attach);
}
EXPORT_SYMBOL(driver_attach);
