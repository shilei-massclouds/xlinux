/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IRQDOMAIN_H
#define _LINUX_IRQDOMAIN_H

#include <of.h>
#include <list.h>
#include <fwnode.h>

#define IRQ_DOMAIN_IRQ_SPEC_PARAMS 16

enum irq_domain_bus_token {
    DOMAIN_BUS_ANY      = 0,
    DOMAIN_BUS_WIRED,
    DOMAIN_BUS_GENERIC_MSI,
    DOMAIN_BUS_PCI_MSI,
    DOMAIN_BUS_PLATFORM_MSI,
    DOMAIN_BUS_NEXUS,
    DOMAIN_BUS_IPI,
    DOMAIN_BUS_FSL_MC_MSI,
    DOMAIN_BUS_TI_SCI_INTA_MSI,
    DOMAIN_BUS_WAKEUP,
};

struct irq_fwspec {
    struct fwnode_handle *fwnode;
    int param_count;
    u32 param[IRQ_DOMAIN_IRQ_SPEC_PARAMS];
};

struct irq_domain {
    struct list_head link;
    const char *name;
    const struct irq_domain_ops *ops;
    struct fwnode_handle *fwnode;
    enum irq_domain_bus_token bus_token;
};

struct irq_domain_ops {
    int (*alloc)(struct irq_domain *d, unsigned int virq,
                 unsigned int nr_irqs, void *arg);
};

unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data);

struct irq_domain *
__irq_domain_add(struct fwnode_handle *fwnode, int size,
                 irq_hw_number_t hwirq_max, int direct_max,
                 const struct irq_domain_ops *ops,
                 void *host_data);

static inline struct fwnode_handle *
of_node_to_fwnode(struct device_node *node)
{
    return node ? &node->fwnode : NULL;
}

static inline struct irq_domain *
irq_domain_add_linear(struct device_node *of_node,
                      unsigned int size,
                      const struct irq_domain_ops *ops,
                      void *host_data)
{
    return __irq_domain_add(of_node_to_fwnode(of_node),
                            size, size, 0, ops, host_data);
}

extern const struct fwnode_operations irqchip_fwnode_ops;

static inline bool is_fwnode_irqchip(struct fwnode_handle *fwnode)
{
    return fwnode && fwnode->ops == &irqchip_fwnode_ops;
}

#endif /* _LINUX_IRQDOMAIN_H */
