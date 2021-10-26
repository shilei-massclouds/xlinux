// SPDX-License-Identifier: GPL-2.0
#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <klist.h>
#include <kobject.h>

struct device;
struct device_driver;

struct bus_type {
    const char      *name;
    const char      *dev_name;
    struct device   *dev_root;

    int (*match)(struct device *dev, struct device_driver *drv);
    int (*probe)(struct device *dev);

    struct subsys_private *p;
};

struct subsys_private {
    struct kset *drivers_kset;
    struct klist klist_devices;
    struct klist klist_drivers;
    unsigned int drivers_autoprobe:1;
    struct bus_type *bus;
};

struct device_private {
    /*
    struct klist klist_children;
    struct klist_node knode_parent;
    */
    struct klist_node knode_driver;
    struct klist_node knode_bus;
    /*
    struct klist_node knode_class;
    struct list_head  deferred_probe;
    */
    struct device *device;
};

#define to_device_private_driver(obj) \
    container_of(obj, struct device_private, knode_driver)
#define to_device_private_bus(obj)  \
    container_of(obj, struct device_private, knode_bus)

struct device {
    struct kobject kobj;

    struct device *parent;

    struct device_private *p;

    const char *init_name; /* initial name of the device */

    struct bus_type *bus;

    void *platform_data;
    struct device_node *of_node; /* associated device tree node */

    struct device_driver *driver;
};

extern int device_add(struct device *dev);

extern void put_device(struct device *dev);

extern int bus_add_device(struct device *dev);

static inline const char *
dev_name(const struct device *dev)
{
    /* Use the init name until the kobject becomes available */
    if (dev->init_name)
        return dev->init_name;

    return kobject_name(&dev->kobj);
}

extern int bus_register(struct bus_type *bus);

int dev_set_name(struct device *dev, const char *fmt, ...);

#endif /* _DEVICE_H_ */
