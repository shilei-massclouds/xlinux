/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IRQDESC_H
#define _LINUX_IRQDESC_H

#include <ptrace.h>
#include <irqdomain.h>
#include <irqhandler.h>

struct irq_desc {
    irq_flow_handler_t handle_irq;
};

/*
 * Convert a HW interrupt number to a logical one using a IRQ domain,
 * and handle the result interrupt number. Return -EINVAL if
 * conversion failed. Providing a NULL domain indicates that the
 * conversion has already been done.
 */
int __handle_domain_irq(struct irq_domain *domain,
                        unsigned int hwirq,
                        bool lookup,
                        struct pt_regs *regs);

static inline int
handle_domain_irq(struct irq_domain *domain,
                  unsigned int hwirq,
                  struct pt_regs *regs)
{
    return __handle_domain_irq(domain, hwirq, true, regs);
}

/*
 * Architectures call this to let the generic IRQ layer
 * handle an interrupt.
 */
static inline void generic_handle_irq_desc(struct irq_desc *desc)
{
    desc->handle_irq(desc);
}

struct irq_desc *irq_to_desc(unsigned int irq);

#endif /* _LINUX_IRQDESC_H */
