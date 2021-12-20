// SPDX-License-Identifier: GPL-2.0

#include <bug.h>
#include <export.h>
#include <irqdesc.h>

struct irq_data *irq_get_irq_data(unsigned int irq)
{
    struct irq_desc *desc = irq_to_desc(irq);

    return desc ? &desc->irq_data : NULL;
}
EXPORT_SYMBOL(irq_get_irq_data);

void handle_fasteoi_irq(struct irq_desc *desc)
{
    panic("%s: !", __func__);
}
EXPORT_SYMBOL(handle_fasteoi_irq);
