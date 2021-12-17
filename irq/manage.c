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
