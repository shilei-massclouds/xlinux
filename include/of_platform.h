// SPDX-License-Identifier: GPL-2.0-only
#ifndef OF_PLATFORM_H
#define OF_PLATFORM_H

#include <fdt.h>
#include <device.h>

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
};

#endif /* OF_PLATFORM_H */
