// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <export.h>
#include <printk.h>

/**
 * irq_enter - Enter an interrupt context including RCU update
 */
void irq_enter(void)
{
    /* Todo */
}
EXPORT_SYMBOL(irq_enter);

static inline void __irq_exit_rcu(void)
{
    panic("%s: !", __func__);
    /*
    if (!in_interrupt() && local_softirq_pending())
        invoke_softirq();
        */
}

/**
 * irq_exit - Exit an interrupt context, update RCU and lockdep
 *
 * Also processes softirqs if needed and possible.
 */
void irq_exit(void)
{
    __irq_exit_rcu();
}
EXPORT_SYMBOL(irq_exit);

static int
init_module(void)
{
    printk("module[softirq]: init begin ...\n");
    printk("module[softirq]: init end!\n");
    return 0;
}
