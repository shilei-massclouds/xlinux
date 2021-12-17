// SPDX-License-Identifier: GPL-2.0-only

#include <irq.h>
#include <errno.h>
#include <export.h>
#include <printk.h>

extern void (*handle_arch_irq)(struct pt_regs *);

int
set_handle_irq(void (*handle_irq)(struct pt_regs *))
{
    if (handle_arch_irq)
        return -EBUSY;

    handle_arch_irq = handle_irq;
    return 0;
}
EXPORT_SYMBOL(set_handle_irq);
