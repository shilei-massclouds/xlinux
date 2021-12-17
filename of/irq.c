// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <errno.h>
#include <export.h>
#include <of_irq.h>
#include <irqdomain.h>

struct device_node *of_irq_find_parent(struct device_node *child)
{
    phandle parent;
    struct device_node *p;

    if (!of_node_get(child))
        return NULL;

    do {
        if (of_property_read_u32(child, "interrupt-parent", &parent))
            p = of_get_parent(child);
        else
            p = of_find_node_by_phandle(parent);
        of_node_put(child);
        child = p;
    } while (p && of_get_property(p, "#interrupt-cells", NULL) == NULL);

    return p;
}
EXPORT_SYMBOL(of_irq_find_parent);

static void *
of_find_property_value_of_size(const struct device_node *np,
                               const char *propname,
                               u32 min,
                               u32 max,
                               size_t *len)
{
    struct property *prop = of_find_property(np, propname, NULL);

    if (!prop)
        return ERR_PTR(-EINVAL);
    if (!prop->value)
        return ERR_PTR(-ENODATA);
    if (prop->length < min)
        return ERR_PTR(-EOVERFLOW);
    if (max && prop->length > max)
        return ERR_PTR(-EOVERFLOW);

    if (len)
        *len = prop->length;

    return prop->value;
}

int
of_irq_parse_one(struct device_node *device,
                 int index,
                 struct of_phandle_args *out_irq)
{
    int i, res;
    u32 intsize;
    struct device_node *p;

    /* Look for the interrupt parent. */
    p = of_irq_find_parent(device);
    if (p == NULL)
        return -EINVAL;

    /* Get size of interrupt specifier */
    if (of_property_read_u32(p, "#interrupt-cells", &intsize))
        return -EINVAL;

    /* Copy intspec into irq structure */
    out_irq->np = p;
    out_irq->args_count = intsize;
    for (i = 0; i < intsize; i++) {
        res = of_property_read_u32_index(device, "interrupts",
                                         (index * intsize) + i,
                                         out_irq->args + i);
        if (res)
            return res;
    }

    return res;
}

int of_irq_get(struct device_node *dev, int index)
{
    int rc;
    struct of_phandle_args oirq;

    rc = of_irq_parse_one(dev, index, &oirq);
    if (rc)
        return rc;

    return irq_create_of_mapping(&oirq);
}
EXPORT_SYMBOL(of_irq_get);
