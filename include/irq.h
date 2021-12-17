/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _IRQ_H_
#define _IRQ_H_

#include <ptrace.h>

#define NR_IRQS 64

int set_handle_irq(void (*handle_irq)(struct pt_regs *));

#endif /* _IRQ_H_ */
