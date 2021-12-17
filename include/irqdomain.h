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

unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data);

#endif /* _LINUX_IRQDOMAIN_H */
