/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _LINUX_KLIST_H
#define _LINUX_KLIST_H

#include <list.h>

struct klist_node {
    void            *n_klist;   /* never access directly */
    list_head       n_node;
};

struct klist {
    list_head k_list;
    void (*get)(struct klist_node *);
    void (*put)(struct klist_node *);
} __attribute__ ((aligned (sizeof(void *))));

extern void klist_init(struct klist *k,
                       void (*get)(struct klist_node *),
                       void (*put)(struct klist_node *));

extern void klist_add_tail(struct klist_node *n, struct klist *k);

#endif /* _LINUX_KLIST_H */
