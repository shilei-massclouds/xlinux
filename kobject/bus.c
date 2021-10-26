// SPDX-License-Identifier: GPL-2.0
#include <klist.h>
#include <slab.h>
#include <device.h>
#include <driver.h>
#include <errno.h>
#include <export.h>
#include <printk.h>

static struct bus_type *
bus_get(struct bus_type *bus)
{
    if (bus) {
        return bus;
    }
    return NULL;
}

int
bus_add_device(struct device *dev)
{
    struct bus_type *bus = bus_get(dev->bus);
    if (bus) {
        printk("bus: '%s': add device %s\n", bus->name, dev_name(dev));
        klist_add_tail(&dev->p->knode_bus, &bus->p->klist_devices);
    }
    return 0;
}

int
bus_register(struct bus_type *bus)
{
    int retval;
    struct subsys_private *priv;

    priv = kzalloc(sizeof(struct subsys_private), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->bus = bus;
    bus->p = priv;

    priv->drivers_autoprobe = 1;

    priv->drivers_kset = kset_create_and_add("drivers");
    if (!priv->drivers_kset) {
        retval = -ENOMEM;
        goto error;
    }

    klist_init(&priv->klist_devices);
    klist_init(&priv->klist_drivers);

    printk("bus: '%s': registered\n", bus->name);
    return 0;

 error:
    kfree(bus->p);
    bus->p = NULL;
    return retval;
}
EXPORT_SYMBOL(bus_register);

int
bus_add_driver(struct device_driver *drv)
{
    struct bus_type *bus;
    struct driver_private *priv;
    int error = 0;

    bus = bus_get(drv->bus);
    if (!bus)
        return -EINVAL;

    printk("bus: '%s': add driver %s\n", bus->name, drv->name);

    priv = kzalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        panic("no memory!");
    }

    klist_init(&priv->klist_devices);
    priv->driver = drv;
    drv->p = priv;
    priv->kobj.kset = bus->p->drivers_kset;

    klist_add_tail(&priv->knode_bus, &bus->p->klist_drivers);
    if (drv->bus->p->drivers_autoprobe) {
        error = driver_attach(drv);
        if (error)
            panic("can not attach driver!");
    }

    panic("%s: bus(%s)", __func__, bus->name);
    return 0;
}
EXPORT_SYMBOL(bus_add_driver);
