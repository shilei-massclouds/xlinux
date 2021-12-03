// SPDX-License-Identifier: GPL-2.0-only

#include <errno.h>
#include <export.h>
#include <printk.h>
#include <xarray.h>

static void *set_bounds(struct xa_state *xas)
{
    xas->xa_node = XAS_BOUNDS;
    return NULL;
}

static void *xas_start(struct xa_state *xas)
{
    void *entry;

    if (xas_valid(xas))
        return xas_reload(xas);
    if (xas_error(xas))
        return NULL;

    entry = xa_head(xas->xa);
    if (!xa_is_node(entry)) {
        if (xas->xa_index)
            return set_bounds(xas);
    } else {
        if ((xas->xa_index >> xa_to_node(entry)->shift) > XA_CHUNK_MASK)
            return set_bounds(xas);
    }
    printk("%s: %p\n", __func__, entry);

    xas->xa_node = NULL;
    return entry;
}

static void *xas_descend(struct xa_state *xas, struct xa_node *node)
{
    panic("%s: !", __func__);
}

void *xas_load(struct xa_state *xas)
{
    void *entry = xas_start(xas);

    while (xa_is_node(entry)) {
        struct xa_node *node = xa_to_node(entry);

        if (xas->xa_shift > node->shift)
            break;
        entry = xas_descend(xas, node);
        if (node->shift == 0)
            break;
    }
    return entry;
}
EXPORT_SYMBOL(xas_load);

static unsigned long xas_max(struct xa_state *xas)
{
    unsigned long max = xas->xa_index;
    return max;
}

/*
 * xas_expand adds nodes to the head of the tree until it has reached
 * sufficient height to be able to contain @xas->xa_index
 */
static int xas_expand(struct xa_state *xas, void *head)
{
    unsigned int shift = 0;
    unsigned long max = xas_max(xas);

    if (!head) {
        if (max == 0)
            return 0;
        while ((max >> shift) >= XA_CHUNK_SIZE)
            shift += XA_CHUNK_SHIFT;
        return shift + XA_CHUNK_SHIFT;
    }

    panic("%s: !", __func__);
}

static void *xas_create(struct xa_state *xas, bool allow_root)
{
    int shift;
    void *entry;
    void **slot;
    struct xarray *xa = xas->xa;
    struct xa_node *node = xas->xa_node;
    unsigned int order = xas->xa_shift;

    if (xas_top(node)) {
        entry = xa_head_locked(xa);
        xas->xa_node = NULL;
        shift = xas_expand(xas, entry);
        if (shift < 0)
            return NULL;
        if (!shift && !allow_root)
            shift = XA_CHUNK_SHIFT;
        entry = xa_head_locked(xa);
        slot = &xa->xa_head;
    } else {
        panic("%s: not top node!", __func__);
    }

    while (shift > order) {
        panic("%s: shift(%d) order(%d)", __func__, shift, order);
    }

    return entry;
}

static void xas_squash_marks(const struct xa_state *xas)
{
    panic("%s: no func!", __func__);
}

static void
update_node(struct xa_state *xas,
            struct xa_node *node,
            int count, int values)
{
    if (!node || (!count && !values))
        return;
 
    panic("%s: node(%p)!", __func__, node);
    /*
    node->count += count;
    node->nr_values += values;
    XA_NODE_BUG_ON(node, node->count > XA_CHUNK_SIZE);
    XA_NODE_BUG_ON(node, node->nr_values > XA_CHUNK_SIZE);
    xas_update(xas, node);
    if (count < 0)
        xas_delete_node(xas);
        */
}

void *xas_store(struct xa_state *xas, void *entry)
{
    void *first, *next;
    struct xa_node *node;
    unsigned int offset, max;
    int count = 0;
    int values = 0;
    void **slot = &xas->xa->xa_head;

    printk("%s: entry(%p)\n", __func__, entry);
    if (entry) {
        bool allow_root = !xa_is_node(entry) && !xa_is_zero(entry);
        first = xas_create(xas, allow_root);
        printk("%s: entry(%p) allow_root(%d)\n", __func__, entry, allow_root);
    } else {
        first = xas_load(xas);
    }

    if (xas_invalid(xas))
        return first;
    node = xas->xa_node;
    if (node && (xas->xa_shift < node->shift))
        xas->xa_sibs = 0;
    if ((first == entry) && !xas->xa_sibs)
        return first;

    next = first;
    offset = xas->xa_offset;
    max = xas->xa_offset + xas->xa_sibs;
    if (node) {
        slot = &node->slots[offset];
        if (xas->xa_sibs)
            xas_squash_marks(xas);
    }
    /*
    if (!entry)
        xas_init_marks(xas);
        */

    for (;;) {
        *slot = entry;
        /*
        if (xa_is_node(next) && (!node || node->shift))
            xas_free_nodes(xas, xa_to_node(next));
        */
        printk("%s: node(%p)\n", __func__, node);
        if (!node)
            break;
        /*
        count += !next - !entry;
        values += !xa_is_value(first) - !value;
        if (entry) {
            if (offset == max)
                break;
            if (!xa_is_sibling(entry))
                entry = xa_mk_sibling(xas->xa_offset);
        } else {
            if (offset == XA_CHUNK_MASK)
                break;
        }
        next = xa_entry_locked(xas->xa, node, ++offset);
        if (!xa_is_sibling(next)) {
            if (!entry && (offset > max))
                break;
            first = next;
        }
        */
        slot++;
    }

    update_node(xas, node, count, values);
    printk("%s: ret first(%p)\n", __func__, first);
    return first;
}
EXPORT_SYMBOL(xas_store);

static void xas_destroy(struct xa_state *xas)
{
    struct xa_node *node = xas->xa_alloc;

    if (!node)
        return;

    panic("%s: !", __func__);
}

bool xas_nomem(struct xa_state *xas, gfp_t gfp)
{
    if (xas->xa_node != XA_ERROR(-ENOMEM)) {
        xas_destroy(xas);
        return false;
    }
    /*
    if (xas->xa->xa_flags & XA_FLAGS_ACCOUNT)
        gfp |= __GFP_ACCOUNT;
    xas->xa_alloc = kmem_cache_alloc(radix_tree_node_cachep, gfp);
    if (!xas->xa_alloc)
        return false;
    BUG_ON(!list_empty(&xas->xa_alloc->private_list));
    xas->xa_node = XAS_RESTART;
    */
    return true;
}
EXPORT_SYMBOL(xas_nomem);

static int
init_module(void)
{
    printk("module[xarray]: init begin ...\n");
    printk("module[xarray]: init end!\n");
    return 0;
}
