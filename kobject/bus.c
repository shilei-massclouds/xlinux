// SPDX-License-Identifier: GPL-2.0
#include <printk.h>
#include <errno.h>
#include <klist.h>
#include <device.h>
#include <export.h>
#include <memblock.h>

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
        printk("bus: '%s': add device %s\n",
                   bus->name, dev_name(dev));
        klist_add_tail(&dev->p->knode_bus, &bus->p->klist_devices);
    }
    return 0;
}

static void
klist_devices_get(struct klist_node *n)
{
}

static void
klist_devices_put(struct klist_node *n)
{
}

int
bus_register(struct bus_type *bus)
{
    int retval;
    struct subsys_private *priv;

    priv = memblock_alloc(sizeof(struct subsys_private), 8);
    if (!priv)
        return -ENOMEM;

    priv->bus = bus;
    bus->p = priv;

    klist_init(&priv->klist_devices,
               klist_devices_get,
               klist_devices_put);

    printk("bus: '%s': registered\n", bus->name);
    return 0;

}
EXPORT_SYMBOL(bus_register);
