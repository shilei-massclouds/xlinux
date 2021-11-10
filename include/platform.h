// SPDX-License-Identifier: GPL-2.0-only
#ifndef OF_PLATFORM_H
#define OF_PLATFORM_H

#include <fdt.h>
#include <device.h>
#include <driver.h>
#include <ioport.h>

#define PLATFORM_DEVID_NONE (-1)
#define PLATFORM_DEVID_AUTO (-2)

struct of_device_id {
    char        name[32];
    char        type[32];
    char        compatible[128];
    const void  *data;
};

struct of_dev_auxdata {
    char *compatible;
    resource_size_t phys_addr;
    char *name;
    void *platform_data;
};

struct platform_device {
    const char *name;
    int id;

    struct device dev;

    u32 num_resources;
    struct resource *resource;
};

#define to_platform_device(x) \
    container_of((x), struct platform_device, dev)

struct platform_driver {
    int (*probe)(struct platform_device *);
    struct device_driver driver;
};

#define to_platform_driver(drv) \
    (container_of((drv), struct platform_driver, driver))

extern struct bus_type platform_bus_type;

int
platform_bus_init(void);

int
platform_driver_register(struct platform_driver *);

const struct of_device_id *
of_match_device(const struct of_device_id *matches,
                const struct device *dev);

static inline int
of_driver_match_device(struct device *dev,
                       const struct device_driver *drv)
{
    return of_match_device(drv->of_match_table, dev) != NULL;
}

void *
devm_platform_ioremap_resource(struct platform_device *pdev,
                               unsigned int index);

int
of_address_to_resource(struct device_node *dev,
                       int index,
                       struct resource *r);

struct device_node *
of_get_parent(const struct device_node *node);

void
platform_device_put(struct platform_device *pdev);

struct resource *
platform_get_resource(struct platform_device *dev,
                      unsigned int type,
                      unsigned int num);

#endif /* OF_PLATFORM_H */
