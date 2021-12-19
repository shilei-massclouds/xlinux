/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _IRQ_H_
#define _IRQ_H_

#include <ptrace.h>
#include <mod_devicetable.h>

#define NR_IRQS 64

#define for_each_matching_node_and_match(dn, matches, match) \
    for (dn = of_find_matching_node_and_match(NULL, matches, match); \
         dn; dn = of_find_matching_node_and_match(dn, matches, match))

int set_handle_irq(void (*handle_irq)(struct pt_regs *));

struct device_node *
of_find_matching_node_and_match(struct device_node *from,
                                const struct of_device_id *matches,
                                const struct of_device_id **match);

#endif /* _IRQ_H_ */
