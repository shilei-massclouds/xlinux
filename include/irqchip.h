// SPDX-License-Identifier: GPL-2.0
#ifndef _IRQ_CHIP_H_
#define _IRQ_CHIP_H_

#include <irqdesc.h>

struct irq_data *irq_get_irq_data(unsigned int irq);

void handle_fasteoi_irq(struct irq_desc *desc);

#endif /* _IRQ_CHIP_H_ */
