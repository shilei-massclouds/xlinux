// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <irq.h>
#include <slab.h>
#include <errno.h>
#include <export.h>
#include <of_irq.h>
#include <irqdomain.h>

struct of_intc_desc {
    struct list_head    list;
    of_irq_init_cb_t    irq_init_cb;
    struct device_node  *dev;
    struct device_node  *interrupt_parent;
};

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

void of_irq_init(const struct of_device_id *matches)
{
    struct device_node *np;
    struct of_intc_desc *desc;
    struct list_head intc_desc_list;
    const struct of_device_id *match;

    INIT_LIST_HEAD(&intc_desc_list);

    for_each_matching_node_and_match(np, matches, &match) {
        if (!of_property_read_bool(np, "interrupt-controller") ||
            !of_device_is_available(np))
            continue;

        BUG_ON(!match->data);

        /*
         * Here, we allocate and populate an of_intc_desc with the node
         * pointer, interrupt-parent device_node etc.
         */
        desc = kzalloc(sizeof(*desc), GFP_KERNEL);
        if (!desc)
            panic("out of memory!");

        desc->irq_init_cb = match->data;
        desc->dev = of_node_get(np);
        desc->interrupt_parent = of_irq_find_parent(np);
        if (desc->interrupt_parent == np)
            desc->interrupt_parent = NULL;
        list_add_tail(&desc->list, &intc_desc_list);
    }

    while (!list_empty(&intc_desc_list)) {
        struct of_intc_desc *temp_desc;
        list_for_each_entry_safe(desc, temp_desc, &intc_desc_list, list) {
            int ret;

            list_del(&desc->list);

            of_node_set_flag(desc->dev, OF_POPULATED);

            ret = desc->irq_init_cb(desc->dev, desc->interrupt_parent);
            if (ret)
                panic("init irq err!");
        }
    }
}
EXPORT_SYMBOL(of_irq_init);
