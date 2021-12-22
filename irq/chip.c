// SPDX-License-Identifier: GPL-2.0

#include <bug.h>
#include <errno.h>
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

/**
 *  irq_set_chip_data - set irq chip data for an irq
 *  @irq:   Interrupt number
 *  @data:  Pointer to chip specific data
 *
 *  Set the hardware irq chip data for an irq
 */
int irq_set_chip_data(unsigned int irq, void *data)
{
    struct irq_desc *desc = irq_to_desc(irq);

    if (!desc)
        return -EINVAL;

    desc->irq_data.chip_data = data;
    return 0;
}
EXPORT_SYMBOL(irq_set_chip_data);

void irq_percpu_enable(struct irq_desc *desc)
{
    desc->irq_data.chip->irq_unmask(&desc->irq_data);
}

void handle_percpu_devid_irq(struct irq_desc *desc)
{
    panic("%s: !", __func__);
}
EXPORT_SYMBOL(handle_percpu_devid_irq);
