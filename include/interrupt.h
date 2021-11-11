/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _LINUX_INTERRUPT_H
#define _LINUX_INTERRUPT_H

#define IRQ_AFFINITY_MAX_SETS  4

/**
 * struct irq_affinity - Description for automatic irq affinity assignements
 * @pre_vectors:    Don't apply affinity to @pre_vectors at beginning of
 *          the MSI(-X) vector space
 * @post_vectors:   Don't apply affinity to @post_vectors at end of
 *          the MSI(-X) vector space
 * @nr_sets:        The number of interrupt sets for which affinity
 *          spreading is required
 * @set_size:       Array holding the size of each interrupt set
 * @calc_sets:      Callback for calculating the number and size
 *          of interrupt sets
 * @priv:       Private data for usage by @calc_sets, usually a
 *          pointer to driver/device specific data.
 */
struct irq_affinity {
    unsigned int    pre_vectors;
    unsigned int    post_vectors;
    unsigned int    nr_sets;
    unsigned int    set_size[IRQ_AFFINITY_MAX_SETS];
    void            (*calc_sets)(struct irq_affinity *, unsigned int nvecs);
    void            *priv;
};

#endif /* _LINUX_INTERRUPT_H */
