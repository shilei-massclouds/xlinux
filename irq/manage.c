// SPDX-License-Identifier: GPL-2.0

#include <bug.h>
#include <errno.h>
#include <export.h>
#include <irqdesc.h>
#include <interrupt.h>

int
request_threaded_irq(unsigned int irq,
                     irq_handler_t handler,
                     irq_handler_t thread_fn,
                     unsigned long irqflags,
                     const char *devname,
                     void *dev_id)
{
    struct irq_desc *desc;

    desc = irq_to_desc(irq);
    if (!desc)
        return -EINVAL;

    //retval = __setup_irq(irq, desc, action);
    panic("%s: !", __func__);
}
EXPORT_SYMBOL(request_threaded_irq);

int
irq_do_set_affinity(struct irq_data *data,
                    const struct cpumask *mask,
                    bool force)
{
    struct irq_chip *chip = irq_data_get_irq_chip(data);

    return chip->irq_set_affinity(data, mask, force);
}

static int
irq_try_set_affinity(struct irq_data *data,
                     const struct cpumask *dest,
                     bool force)
{
    return irq_do_set_affinity(data, dest, force);
}

int
irq_set_affinity_locked(struct irq_data *data,
                        const struct cpumask *mask,
                        bool force)
{
    return irq_try_set_affinity(data, mask, force);
}

int __irq_set_affinity(unsigned int irq,
                       const struct cpumask *mask,
                       bool force)
{
    struct irq_desc *desc = irq_to_desc(irq);

    if (!desc)
        return -EINVAL;

    printk("%s: 1 irq(%u)\n", __func__, irq);
    printk("%s: 2 irq(%u) (%u)\n", __func__, irq, desc->irq_data.irq);
    return irq_set_affinity_locked(irq_desc_get_irq_data(desc), mask, force);
}
EXPORT_SYMBOL(__irq_set_affinity);
