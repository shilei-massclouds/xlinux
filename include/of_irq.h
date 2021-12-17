/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __OF_IRQ_H
#define __OF_IRQ_H

#include <fdt.h>

int of_irq_get(struct device_node *dev, int index);

#endif /* __OF_IRQ_H */
