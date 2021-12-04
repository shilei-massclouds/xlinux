// SPDX-License-Identifier: GPL-2.0-only

#include <slab.h>
#include <errno.h>
#include <export.h>
#include <printk.h>
#include <string.h>
#include <xarray.h>

struct kmem_cache *xa_node_cachep;

static void *set_bounds(struct xa_state *xas)
{
    xas->xa_node = XAS_BOUNDS;
    return NULL;
}

static void *xas_start(struct xa_state *xas)
{
    void *entry;

    printk("%s: 1 %p\n", __func__, entry);
    if (xas_valid(xas))
        return xas_reload(xas);
    if (xas_error(xas))
        return NULL;

    printk("%s: 2 %p\n", __func__, entry);
    entry = xa_head(xas->xa);
    if (!xa_is_node(entry)) {
        printk("%s: 3 %d\n", __func__, xas->xa_index);
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

/* extracts the offset within this node from the index */
static unsigned int get_offset(unsigned long index, struct xa_node *node)
{
    return (index >> node->shift) & XA_CHUNK_MASK;
}

static void *xas_descend(struct xa_state *xas, struct xa_node *node)
{
    unsigned int offset = get_offset(xas->xa_index, node);
    void *entry = xa_entry(xas->xa, node, offset);

    xas->xa_node = node;
    xas->xa_offset = offset;
    return entry;
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
    return xas->xa_index;
}

/* The maximum index that can be contained in the array without expanding it */
static unsigned long max_index(void *entry)
{
    if (!xa_is_node(entry))
        return 0;
    return (XA_CHUNK_SIZE << xa_to_node(entry)->shift) - 1;
}

static void *xas_alloc(struct xa_state *xas, unsigned int shift)
{
    struct xa_node *parent = xas->xa_node;
    struct xa_node *node = xas->xa_alloc;

    if (xas_invalid(xas))
        return NULL;

    if (node) {
        xas->xa_alloc = NULL;
    } else {
        gfp_t gfp = GFP_NOWAIT | __GFP_NOWARN;

        node = kmem_cache_alloc(xa_node_cachep, gfp);
        if (!node) {
            xas_set_err(xas, -ENOMEM);
            return NULL;
        }
    }

    if (parent) {
        node->offset = xas->xa_offset;
    }

    BUG_ON(shift > BITS_PER_LONG);
    BUG_ON(!list_empty(&node->private_list));
    node->shift = shift;
    //node->count = 0;
    //node->nr_values = 0;
    node->parent = xas->xa_node;
    node->array = xas->xa;
    return node;
}

/*
 * xas_expand adds nodes to the head of the tree until it has reached
 * sufficient height to be able to contain @xas->xa_index
 */
static int xas_expand(struct xa_state *xas, void *head)
{
    unsigned int shift = 0;
    struct xa_node *node = NULL;
    struct xarray *xa = xas->xa;
    unsigned long max = xas_max(xas);

    printk("%s: head(%p) max(%lu) xa(%p)\n",
           __func__, head, max, xas->xa);
    if (!head) {
        printk("%s: 1\n", __func__);
        if (max == 0)
            return 0;
        while ((max >> shift) >= XA_CHUNK_SIZE)
            shift += XA_CHUNK_SHIFT;
        return shift + XA_CHUNK_SHIFT;
    } else if (xa_is_node(head)) {
        node = xa_to_node(head);
        shift = node->shift + XA_CHUNK_SHIFT;
    }
    xas->xa_node = NULL;

    while (max > max_index(head)) {
        printk("%s: shift(%d) max(%lu, %lu)\n",
               __func__, shift, max, max_index(head));
        BUG_ON(shift > BITS_PER_LONG);
        node = xas_alloc(xas, shift);
        if (!node)
            return -ENOMEM;

        //node->count = 1;
        BUG_ON(xa_is_value(head));
        node->slots[0] = head;

        if (xa_is_node(head)) {
            //xa_to_node(head)->offset = 0;
            xa_to_node(head)->parent = node;
        }
        head = xa_mk_node(node);
        xa->xa_head = head;
        //xas_update(xas, node);

        shift += XA_CHUNK_SHIFT;
    }

    xas->xa_node = node;
    return shift;
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
        printk("%s: 1 entry(%p)\n", __func__, entry);
        xas->xa_node = NULL;
        shift = xas_expand(xas, entry);
        printk("%s: 2 shift(%d)\n", __func__, shift);
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
        shift -= XA_CHUNK_SHIFT;
        if (!entry) {
            node = xas_alloc(xas, shift);
            if (!node)
                break;
            *slot = xa_mk_node(node);
        } else if (xa_is_node(entry)) {
            node = xa_to_node(entry);
        } else {
            break;
        }
        entry = xas_descend(xas, node);
        slot = &node->slots[xas->xa_offset];

        printk("%s: shift(%d) order(%d), xa_offset(%lu)",
               __func__, shift, order, xas->xa_offset);
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

    printk("%s: 1 entry(%p)\n", __func__, entry);
    if (entry) {
        bool allow_root = !xa_is_node(entry) && !xa_is_zero(entry);
        first = xas_create(xas, allow_root);
        printk("%s: entry(%p) allow_root(%d) first(%p)\n",
               __func__, entry, allow_root, first);
    } else {
        first = xas_load(xas);
    }

    if (xas_invalid(xas))
        return first;

    printk("%s: 2 entry(%p)\n", __func__, entry);
    node = xas->xa_node;
    if (node && (xas->xa_shift < node->shift))
        xas->xa_sibs = 0;
    if ((first == entry) && !xas->xa_sibs)
        return first;

    printk("%s: 3 first(%p) entry(%p)\n", __func__, first, entry);
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
        */
        printk("%s: entry(%p) offset(%lu) max(%lu)",
               __func__, entry, offset, max);
        if (entry) {
            if (offset == max)
                break;
        } else {
            if (offset == XA_CHUNK_MASK)
                break;
        }
        /*
        next = xa_entry_locked(xas->xa, node, ++offset);
        if (!entry && (offset > max))
            break;
        first = next;
        slot++;
        */
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
    xas->xa_alloc = kmem_cache_alloc(xa_node_cachep, gfp);
    if (!xas->xa_alloc)
        return false;
    BUG_ON(!list_empty(&xas->xa_alloc->private_list));
    xas->xa_node = XAS_RESTART;
    */
    return true;
}
EXPORT_SYMBOL(xas_nomem);

static void
xa_node_ctor(void *arg)
{
    struct xa_node *node = arg;

    memset(node, 0, sizeof(*node));
    INIT_LIST_HEAD(&node->private_list);
}

void xarray_init(void)
{
    BUG_ON(XA_CHUNK_SIZE > 255);
    xa_node_cachep = kmem_cache_create("xa_node",
                                       sizeof(struct xa_node), 0,
                                       SLAB_PANIC | SLAB_RECLAIM_ACCOUNT,
                                       xa_node_ctor);
}

static int
init_module(void)
{
    printk("module[xarray]: init begin ...\n");

    xarray_init();

    printk("module[xarray]: init end!\n");
    return 0;
}
