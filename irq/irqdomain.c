// SPDX-License-Identifier: GPL-2.0+

#include <of.h>
#include <bug.h>
#include <slab.h>
#include <export.h>
#include <irqdomain.h>

static LIST_HEAD(irq_domain_list);

const struct fwnode_operations irqchip_fwnode_ops;
EXPORT_SYMBOL(irqchip_fwnode_ops);

struct irq_domain *
irq_find_matching_fwspec(struct irq_fwspec *fwspec,
                         enum irq_domain_bus_token bus_token)
{
    struct irq_domain *h;
    struct fwnode_handle *fwnode = fwspec->fwnode;

    list_for_each_entry(h, &irq_domain_list, link) {
        if (fwnode && (h->fwnode == fwnode) &&
            ((bus_token == DOMAIN_BUS_ANY) ||
             (h->bus_token == bus_token)))
            return h;
    }

    panic("no irq domain!");
}

unsigned int irq_create_fwspec_mapping(struct irq_fwspec *fwspec)
{
    struct irq_domain *domain;

    if (fwspec->fwnode) {
        domain = irq_find_matching_fwspec(fwspec, DOMAIN_BUS_ANY);
    } else {
        panic("fwnode is NULL!");
    }
    panic("%s: !", __func__);
}

static void
of_phandle_args_to_fwspec(struct device_node *np,
                          const u32 *args,
                          unsigned int count,
                          struct irq_fwspec *fwspec)
{
    int i;

    fwspec->fwnode = np ? &np->fwnode : NULL;
    fwspec->param_count = count;

    for (i = 0; i < count; i++)
        fwspec->param[i] = args[i];
}

unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data)
{
    struct irq_fwspec fwspec;

    of_phandle_args_to_fwspec(irq_data->np, irq_data->args,
                              irq_data->args_count, &fwspec);

    return irq_create_fwspec_mapping(&fwspec);
}
EXPORT_SYMBOL(irq_create_of_mapping);

struct irq_domain *
__irq_domain_add(struct fwnode_handle *fwnode, int size,
                 irq_hw_number_t hwirq_max, int direct_max,
                 const struct irq_domain_ops *ops,
                 void *host_data)
{
    struct irq_domain *domain;

    domain = kzalloc(sizeof(*domain) + (sizeof(unsigned int) * size),
                     GFP_KERNEL);
    if (!domain)
        return NULL;

    if (is_fwnode_irqchip(fwnode)) {
        panic("fwnode is NOT irqchip!");
    } else if (is_of_node(fwnode)) {
        printk("is of node!\n");
    } else {
        panic("check fwnode error!");
    }

    domain->ops = ops;

    list_add(&domain->link, &irq_domain_list);
    return domain;
}
EXPORT_SYMBOL(__irq_domain_add);
