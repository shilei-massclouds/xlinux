// SPDX-License-Identifier: GPL-2.0-only

#include <klist.h>
#include <printk.h>
#include <platform.h>

static int
test_platform(void)
{
    struct klist_iter iter;
    struct klist_node *n;
    struct bus_type *bus = &platform_bus_type;

    if (!bus || !bus->p)
        return -1;

    klist_iter_init_node(&bus->p->klist_devices, &iter, NULL);
    while ((n = klist_next(&iter))) {
        int i;
        struct device_private *dev_prv = to_device_private_bus(n);
        struct device *dev = dev_prv->device;
        struct platform_device *pdev = to_platform_device(dev);
        struct device_node *of_node = dev->of_node;
        struct property *property = of_node->properties;
        printk("device: %s.\n", of_node->full_name);
        printk("  of porperties:\n");
        while (property) {
            printk("    [%s]\n", property->name);
            property = property->next;
        }
        printk("  resources (%u):\n", pdev->num_resources);
        for (i = 0; i < pdev->num_resources; i++) {
            printk("    resources (%lx, %lx):\n",
                   pdev->resource[i].start, pdev->resource[i].end);
        }
    }
    klist_iter_exit(&iter);

    return 0;
}

static int
init_module(void)
{
    printk("module[test_platform]: init begin ...\n");

    if(test_platform()) {
        printk(_RED("test platform failed!\n"));
        return -1;
    }

    printk(_GREEN("test platform okay!\n"));

    printk("module[test_platform]: init end!\n");
    return 0;
}
