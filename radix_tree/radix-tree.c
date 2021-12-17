// SPDX-License-Identifier: GPL-2.0-only

#include <export.h>
#include <printk.h>
#include <radix-tree.h>

static inline struct radix_tree_node *entry_to_node(void *ptr)
{
    return (void *)((unsigned long)ptr & ~RADIX_TREE_INTERNAL_NODE);
}

/*
 * The maximum index which can be stored in a radix tree
 */
static inline unsigned long shift_maxindex(unsigned int shift)
{
    return (RADIX_TREE_MAP_SIZE << shift) - 1;
}

static inline unsigned long node_maxindex(const struct radix_tree_node *node)
{
    return shift_maxindex(node->shift);
}

static unsigned
radix_tree_load_root(const struct radix_tree_root *root,
                     struct radix_tree_node **nodep,
                     unsigned long *maxindex)
{
    struct radix_tree_node *node = root->xa_head;

    *nodep = node;

    if (likely(radix_tree_is_internal_node(node))) {
        node = entry_to_node(node);
        *maxindex = node_maxindex(node);
        return node->shift + RADIX_TREE_MAP_SHIFT;
    }

    *maxindex = 0;
    return 0;
}

void *
__radix_tree_lookup(const struct radix_tree_root *root,
                    unsigned long index,
                    struct radix_tree_node **nodep,
                    void ***slotp)
{
    void **slot;
    unsigned long maxindex;
    struct radix_tree_node *node, *parent;

 restart:
    parent = NULL;
    slot = (void **)&root->xa_head;
    radix_tree_load_root(root, &node, &maxindex);
    if (index > maxindex)
        return NULL;

    panic("%s: !", __func__);

    /*
    while (radix_tree_is_internal_node(node)) {
        unsigned offset;

        parent = entry_to_node(node);
        offset = radix_tree_descend(parent, &node, index);
        slot = parent->slots + offset;
        if (node == RADIX_TREE_RETRY)
            goto restart;
        if (parent->shift == 0)
            break;
    }

    if (nodep)
        *nodep = parent;
    if (slotp)
        *slotp = slot;
    return node;
    */
}

void *
radix_tree_lookup(const struct radix_tree_root *root,
                  unsigned long index)
{
    printk("##############%s: \n", __func__);
    return __radix_tree_lookup(root, index, NULL, NULL);
}
EXPORT_SYMBOL(radix_tree_lookup);

static int
init_module(void)
{
    printk("module[radix-tree]: init begin ...\n");
    printk("module[radix-tree]: init end!\n");
    return 0;
}
