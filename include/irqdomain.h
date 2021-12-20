/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IRQDOMAIN_H
#define _LINUX_IRQDOMAIN_H

#include <of.h>
#include <list.h>
#include <fwnode.h>
#include <interrupt.h>
#include <irqhandler.h>

#define IRQ_DOMAIN_IRQ_SPEC_PARAMS 16

/* Irq domain flags */
enum {
    /* Irq domain is hierarchical */
    IRQ_DOMAIN_FLAG_HIERARCHY   = (1 << 0),
};

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
    void *host_data;
    unsigned int flags;
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

int __irq_domain_alloc_irqs(struct irq_domain *domain, int irq_base,
                            unsigned int nr_irqs, int node, void *arg,
                            bool realloc,
                            const struct irq_affinity_desc *affinity);

static inline int
irq_domain_alloc_irqs(struct irq_domain *domain,
                      unsigned int nr_irqs, int node, void *arg)
{
    return __irq_domain_alloc_irqs(domain, -1, nr_irqs, node, arg,
                                   false, NULL);
}

static inline bool irq_domain_is_hierarchy(struct irq_domain *domain)
{
    return domain->flags & IRQ_DOMAIN_FLAG_HIERARCHY;
}

int irq_domain_translate_onecell(struct irq_domain *d,
                                 struct irq_fwspec *fwspec,
                                 unsigned long *out_hwirq,
                                 unsigned int *out_type);

void irq_domain_set_info(struct irq_domain *domain, unsigned int virq,
                         irq_hw_number_t hwirq, struct irq_chip *chip,
                         void *chip_data, irq_flow_handler_t handler,
                         void *handler_data, const char *handler_name);

#endif /* _LINUX_IRQDOMAIN_H */
