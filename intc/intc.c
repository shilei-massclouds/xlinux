// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <csr.h>
#include <irq.h>
#include <printk.h>
#include <irqdesc.h>
#include <irqdomain.h>

static struct irq_domain *intc_domain;

static void
riscv_intc_irq(struct pt_regs *regs)
{
    unsigned long cause = regs->cause & ~CAUSE_IRQ_FLAG;

    if (unlikely(cause >= BITS_PER_LONG))
        panic("unexpected interrupt cause");

    switch (cause) {
    case IRQ_S_SOFT:
        panic("no irq soft!");
        break;
    default:
        handle_domain_irq(intc_domain, cause, regs);
        break;
    }
}

static int
riscv_intc_init(void)
{
    int rc;

    rc = set_handle_irq(&riscv_intc_irq);
    if (rc)
        panic("failed to set irq handler");

    return 0;
}

static int
init_module(void)
{
    printk("module[intc]: init begin ...\n");

    riscv_intc_init();

    printk("module[intc]: init end!\n");
    return 0;
}
