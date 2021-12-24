/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LINUX_HARDIRQ_H
#define LINUX_HARDIRQ_H

/*
 * Enter irq context (on NO_HZ, update jiffies):
 */
void irq_enter(void);

/*
 * Exit irq context and process softirqs if needed:
 */
void irq_exit(void);

#endif /* LINUX_HARDIRQ_H */
