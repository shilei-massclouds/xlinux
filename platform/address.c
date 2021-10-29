// SPDX-License-Identifier: GPL-2.0

#include <fdt.h>
#include <errno.h>
#include <types.h>
#include <export.h>
#include <ioport.h>
#include <platform.h>

/* Max address size we deal with */
#define OF_MAX_ADDR_CELLS   4
#define OF_CHECK_ADDR_COUNT(na) ((na) > 0 && (na) <= OF_MAX_ADDR_CELLS)
#define OF_CHECK_COUNTS(na, ns) (OF_CHECK_ADDR_COUNT(na) && (ns) > 0)

struct of_bus {
    const char *name;
    const char *addresses;
    int (*match)(struct device_node *parent);
    void (*count_cells)(struct device_node *child, int *addrc, int *sizec);
};

static void
of_bus_default_count_cells(struct device_node *dev,
                           int *addrc,
                           int *sizec)
{
    if (addrc)
        *addrc = of_n_addr_cells(dev);
    if (sizec)
        *sizec = of_n_size_cells(dev);
}

static struct of_bus of_busses[] = {
    /* Default */
    {
        .name = "default",
        .addresses = "reg",
        .match = NULL,
        .count_cells = of_bus_default_count_cells,
    },
};

static struct of_bus *
of_match_bus(struct device_node *np)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(of_busses); i++)
        if (!of_busses[i].match || of_busses[i].match(np))
            return &of_busses[i];
    BUG();
    return NULL;
}

const u32 *
of_get_address(struct device_node *dev,
               int index,
               u64 *size,
               unsigned int *flags)
{
    struct device_node *parent;
    struct of_bus *bus;
    int na, ns;

    parent = of_get_parent(dev);
    if (parent == NULL)
        return NULL;

    bus = of_match_bus(parent);
    bus->count_cells(dev, &na, &ns);
    of_node_put(parent);

    panic("%s: parent(%lx:%s) self(%s) (%x, %x)\n",
          __func__, parent, parent->name, dev->name, na, ns);
    if (!OF_CHECK_ADDR_COUNT(na))
        return NULL;
}

int
of_address_to_resource(struct device_node *dev,
                       int index,
                       struct resource *r)
{
    u64 size;
    const u32 *addrp;
    unsigned int flags;
    const char *name = NULL;

    addrp = of_get_address(dev, index, &size, &flags);
    if (addrp == NULL)
        return -EINVAL;

    /* Get optional "reg-names" property to add a name to a resource */
    /*
    of_property_read_string_index(dev, "reg-names", index, &name);

    return __of_address_to_resource(dev, addrp, size, flags, name, r);
    */
}
EXPORT_SYMBOL(of_address_to_resource);
