// SPDX-License-Identifier: GPL-2.0-only

#include <slab.h>
#include <export.h>
#include <of_irq.h>
#include <printk.h>
#include <irqdomain.h>

bool plic_initialized;
EXPORT_SYMBOL(plic_initialized);

struct plic_priv {
    struct irq_domain *irqdomain;
};

static int
plic_irqdomain_map(struct irq_domain *d, unsigned int irq,
                   irq_hw_number_t hwirq)
{
    irq_set_affinity(irq, NULL);
    return 0;
}

static int
plic_irq_domain_alloc(struct irq_domain *domain, unsigned int virq,
                      unsigned int nr_irqs, void *arg)
{
    int i, ret;
    unsigned int type;
    irq_hw_number_t hwirq;
    struct irq_fwspec *fwspec = arg;

    ret = irq_domain_translate_onecell(domain, fwspec, &hwirq, &type);
    if (ret)
        return ret;

    for (i = 0; i < nr_irqs; i++) {
        ret = plic_irqdomain_map(domain, virq + i, hwirq + i);
        if (ret)
            return ret;
    }

    panic("%s: virq(%u) hwirq(%lu)!", __func__, virq, hwirq);
    return 0;
}

static const struct irq_domain_ops plic_irqdomain_ops = {
    .alloc  = plic_irq_domain_alloc,
};

static int
plic_init(struct device_node *node, struct device_node *parent)
{
    u32 nr_irqs;
    struct plic_priv *priv;

    priv = kzalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv)
        panic("out of memory!");

    of_property_read_u32(node, "riscv,ndev", &nr_irqs);
    if (!nr_irqs)
        panic("out iounmap!");

    priv->irqdomain = irq_domain_add_linear(node, nr_irqs + 1,
                                            &plic_irqdomain_ops, priv);
    if (!priv->irqdomain)
        panic("irq domain can not add linear!");

    return 0;
}

static int
init_module(void)
{
    struct of_device_id match = {
        .compatible = "riscv,plic0",
        .data = plic_init,
    };

    printk("module[plic]: init begin ...\n");

    of_irq_init(&match);

    plic_initialized = true;

    printk("module[plic]: init end!\n");
    return 0;
}
