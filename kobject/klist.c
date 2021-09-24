// SPDX-License-Identifier: GPL-2.0
#include <klist.h>
#include <export.h>

static void
knode_set_klist(struct klist_node *knode, struct klist *klist)
{
    knode->n_klist = klist;
}

static void
klist_node_init(struct klist *k, struct klist_node *n)
{
    INIT_LIST_HEAD(&n->n_node);
    knode_set_klist(n, k);
    if (k->get)
        k->get(n);
}

static void
add_tail(struct klist *k, struct klist_node *n)
{
    list_add_tail(&n->n_node, &k->k_list);
}

void
klist_add_tail(struct klist_node *n, struct klist *k)
{
    klist_node_init(k, n);
    add_tail(k, n);
}
EXPORT_SYMBOL(klist_add_tail);

void
klist_init(struct klist *k,
           void (*get)(struct klist_node *),
           void (*put)(struct klist_node *))
{
    INIT_LIST_HEAD(&k->k_list);
    k->get = get;
    k->put = put;
}
EXPORT_SYMBOL(klist_init);
