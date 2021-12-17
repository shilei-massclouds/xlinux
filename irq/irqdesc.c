// SPDX-License-Identifier: GPL-2.0

#include <bug.h>
#include <irq.h>
#include <errno.h>
#include <export.h>
#include <irqdesc.h>
#include <irq_regs.h>
#include <radix-tree.h>

int nr_irqs = NR_IRQS;

static RADIX_TREE(irq_desc_tree, GFP_KERNEL);

struct irq_desc *irq_to_desc(unsigned int irq)
{
    return radix_tree_lookup(&irq_desc_tree, irq);
}
EXPORT_SYMBOL(irq_to_desc);

int generic_handle_irq(unsigned int irq)
{
    struct irq_desc *desc = irq_to_desc(irq);

    if (!desc)
        return -EINVAL;

    generic_handle_irq_desc(desc);
    return 0;
}
EXPORT_SYMBOL(generic_handle_irq);

int
__handle_domain_irq(struct irq_domain *domain,
                    unsigned int hwirq,
                    bool lookup,
                    struct pt_regs *regs)
{
    unsigned int irq = hwirq;
    struct pt_regs *old_regs = set_irq_regs(regs);

    /*
     * Some hardware gives randomly wrong interrupts.  Rather
     * than crashing, do something sensible.
     */
    if (unlikely(!irq || irq >= nr_irqs)) {
        panic("bad irq(%u)", irq);
    } else {
        generic_handle_irq(irq);
    }

    set_irq_regs(old_regs);
    return 0;
}
EXPORT_SYMBOL(__handle_domain_irq);
