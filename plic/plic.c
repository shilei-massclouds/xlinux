// SPDX-License-Identifier: GPL-2.0-only

#include <irq.h>
#include <mmio.h>
#include <slab.h>
#include <export.h>
#include <of_irq.h>
#include <printk.h>
#include <irqchip.h>
#include <irqdomain.h>
#include <of_address.h>

#define PRIORITY_BASE   0
#define PRIORITY_PER_ID 4

struct plic_priv {
    struct irq_domain *irqdomain;
    void *regs;
};

struct plic_handler {
    void *enable_base;
};

bool plic_initialized;
EXPORT_SYMBOL(plic_initialized);

static struct plic_handler plic_handler;

static inline void
plic_toggle(struct plic_handler *handler, int hwirq, int enable)
{
    u32 hwirq_mask = 1 << (hwirq % 32);
    u32 *reg = handler->enable_base + (hwirq / 32) * sizeof(u32);

    if (enable)
        writel(readl(reg) | hwirq_mask, reg);
    else
        writel(readl(reg) & ~hwirq_mask, reg);
}

static inline void
plic_irq_toggle(const struct cpumask *mask, struct irq_data *d, int enable)
{
    struct plic_priv *priv = irq_get_chip_data(d->irq);

    panic("%s: 1 (%u, %u)!", __func__, priv->regs, d->hwirq);

    writel(enable, priv->regs + PRIORITY_BASE + d->hwirq * PRIORITY_PER_ID);
    plic_toggle(&plic_handler, d->hwirq, enable);
}

static int
plic_set_affinity(struct irq_data *d,
                  const struct cpumask *mask_val,
                  bool force)
{
    plic_irq_toggle(NULL, d, 0);
    plic_irq_toggle(NULL, d, 1);

    panic("%s: !", __func__);
    return IRQ_SET_MASK_OK_DONE;
}

static struct irq_chip plic_chip = {
    .name = "SiFive PLIC",
    .irq_set_affinity = plic_set_affinity,
};

static int
plic_irqdomain_map(struct irq_domain *d, unsigned int irq,
                   irq_hw_number_t hwirq)
{
    struct plic_priv *priv = d->host_data;

    printk("######### %s: 0 irq(%u)\n", __func__, irq);
    irq_domain_set_info(d, irq, hwirq, &plic_chip, d->host_data,
                        handle_fasteoi_irq, NULL, NULL);


    printk("%s: 1 irq(%u)\n", __func__, irq);
    irq_set_affinity(irq, NULL);
    printk("%s: 2 irq(%u)\n", __func__, irq);
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
        printk("%s: 1 (%d) virq(%u)\n", __func__, i, virq);
        ret = plic_irqdomain_map(domain, virq + i, hwirq + i);
        printk("%s: 2 (%d) virq(%u)\n", __func__, i, virq);
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

    priv->regs = of_iomap(node, 0);
    if (!priv->regs)
        panic("bad regs!");

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
