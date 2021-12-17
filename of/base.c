// SPDX-License-Identifier: GPL-2.0-only

#include <of.h>
#include <export.h>

struct device_node *of_get_parent(const struct device_node *node)
{
    if (!node)
        return NULL;

    return of_node_get(node->parent);
}
EXPORT_SYMBOL(of_get_parent);

struct device_node *of_find_node_by_phandle(phandle handle)
{
    struct device_node *np = NULL;

    if (!handle)
        return NULL;

    for_each_of_allnodes(np) {
        if (np->phandle == handle &&
            !of_node_check_flag(np, OF_DETACHED)) {
            break;
        }
    }

    of_node_get(np);
    return np;
}
EXPORT_SYMBOL(of_find_node_by_phandle);

struct device_node *__of_find_all_nodes(struct device_node *prev)
{
    struct device_node *np;
    if (!prev) {
        np = of_root;
    } else if (prev->child) {
        np = prev->child;
    } else {
        /* Walk back up looking for a sibling, or the end of the structure */
        np = prev;
        while (np->parent && !np->sibling)
            np = np->parent;
        np = np->sibling; /* Might be null at the end of the tree */
    }
    return np;
}
