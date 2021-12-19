// SPDX-License-Identifier: GPL-2.0+

#include <of.h>
#include <bug.h>
#include <slab.h>
#include <errno.h>
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
    int virq;
    struct irq_domain *domain;

    if (fwspec->fwnode) {
        domain = irq_find_matching_fwspec(fwspec, DOMAIN_BUS_ANY);
    } else {
        panic("fwnode is NULL!");
    }

    if (irq_domain_is_hierarchy(domain)) {
        virq = irq_domain_alloc_irqs(domain, 1, NUMA_NO_NODE, fwspec);
        if (virq <= 0)
            return 0;
    } else {
        panic("not hierarchy!");
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

static void irq_domain_check_hierarchy(struct irq_domain *domain)
{
    /* Hierarchy irq_domains must implement callback alloc() */
    if (domain->ops->alloc)
        domain->flags |= IRQ_DOMAIN_FLAG_HIERARCHY;
}

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
        char *name;

        /*
         * fwnode paths contain '/', which debugfs is legitimately
         * unhappy about. Replace them with ':', which does
         * the trick and is not as offensive as '\'...
         */
        name = kasprintf(GFP_KERNEL, "%pfw", fwnode);
        if (!name) {
            kfree(domain);
            return NULL;
        }

        strreplace(name, '/', ':');

        domain->name = name;
        domain->fwnode = fwnode;
    } else {
        panic("check fwnode error!");
    }

    domain->ops = ops;
    irq_domain_check_hierarchy(domain);

    list_add(&domain->link, &irq_domain_list);
    return domain;
}
EXPORT_SYMBOL(__irq_domain_add);

int
irq_domain_alloc_irqs_hierarchy(struct irq_domain *domain,
                                unsigned int irq_base,
                                unsigned int nr_irqs, void *arg)
{
    if (!domain->ops->alloc) {
        panic("domain->ops->alloc() is NULL");
        return -ENOSYS;
    }

    return domain->ops->alloc(domain, irq_base, nr_irqs, arg);
}

int
__irq_domain_alloc_irqs(struct irq_domain *domain, int irq_base,
                        unsigned int nr_irqs, int node, void *arg,
                        bool realloc,
                        const struct irq_affinity_desc *affinity)
{
    int i, ret, virq;

    panic("%s: !", __func__);
    ret = irq_domain_alloc_irqs_hierarchy(domain, virq, nr_irqs, arg);

    return virq;
}
