/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _LINUX_XARRAY_H
#define _LINUX_XARRAY_H

#include <bug.h>
#include <list.h>
#include <kernel.h>

#define XA_CHUNK_SHIFT  6
#define XA_CHUNK_SIZE   (1UL << XA_CHUNK_SHIFT)
#define XA_CHUNK_MASK   (XA_CHUNK_SIZE - 1)
#define XA_MAX_MARKS    3
#define XA_MARK_LONGS   DIV_ROUND_UP(XA_CHUNK_SIZE, BITS_PER_LONG)

/*
 * We encode errnos in the xas->xa_node.  If an error has happened, we need to
 * drop the lock to fix it, and once we've done so the xa_state is invalid.
 */
#define XA_ERROR(errno) ((struct xa_node *)(((unsigned long)errno << 2) | 2UL))
#define XAS_BOUNDS  ((struct xa_node *)1UL)
#define XAS_RESTART ((struct xa_node *)3UL)

/**
 * XA_STATE() - Declare an XArray operation state.
 * @name: Name of this operation state (usually xas).
 * @array: Array to operate on.
 * @index: Initial index of interest.
 *
 * Declare and initialise an xa_state on the stack.
 */
#define XA_STATE(name, array, index) \
    struct xa_state name = __XA_STATE(array, index, 0)

struct xa_node {
    unsigned char   shift;  /* Bits remaining in each slot */
    unsigned char   offset; /* Slot offset in parent */

    struct xa_node  *parent;    /* NULL at top of tree */
    struct xarray   *array;     /* The array we belong to */

    void *slots[XA_CHUNK_SIZE];
};

struct xarray {
    void *xa_head;
};

#define XARRAY_INIT(name, flags) {  \
    .xa_head = NULL,                \
}

struct xa_state {
    struct xarray *xa;
    unsigned long xa_index;
    unsigned char xa_shift;
    unsigned char xa_offset;
    struct xa_node *xa_node;
};

#define __XA_STATE(array, index, shift)  {    \
    .xa = array,                    \
    .xa_index = index,              \
    .xa_shift = shift,              \
    .xa_offset = 0,                 \
    .xa_node = XAS_RESTART,         \
}

static inline void *
xa_entry(const struct xarray *xa,
         const struct xa_node *node,
         unsigned int offset)
{
    BUG_ON(offset >= XA_CHUNK_SIZE);
    return node->slots[offset];
}

static inline void *
xa_entry_locked(const struct xarray *xa,
                const struct xa_node *node,
                unsigned int offset)
{
    BUG_ON(offset >= XA_CHUNK_SIZE);
    return node->slots[offset];
}

static inline void *xa_mk_internal(unsigned long v)
{
    return (void *)((v << 2) | 2);
}

static inline bool xa_is_internal(const void *entry)
{
    return ((unsigned long)entry & 3) == 2;
}

static inline bool xa_is_err(const void *entry)
{
    return unlikely(xa_is_internal(entry) &&
                    entry >= xa_mk_internal(-MAX_ERRNO));
}

static inline int xa_err(void *entry)
{
    /* xa_to_internal() would not do sign extension. */
    if (xa_is_err(entry))
        return (long)entry >> 2;
    return 0;
}

static inline int xas_error(const struct xa_state *xas)
{
    return xa_err(xas->xa_node);
}

static inline bool xas_invalid(const struct xa_state *xas)
{
    return (unsigned long)xas->xa_node & 3;
}

static inline bool xas_valid(const struct xa_state *xas)
{
    return !xas_invalid(xas);
}

static inline void *xa_head(const struct xarray *xa)
{
    return xa->xa_head;
}

static inline void *xa_head_locked(const struct xarray *xa)
{
    return xa->xa_head;
}

static inline bool xa_is_node(const void *entry)
{
    return xa_is_internal(entry) && (unsigned long)entry > 4096;
}

#define XA_ZERO_ENTRY   xa_mk_internal(257)

static inline bool xa_is_zero(const void *entry)
{
    return unlikely(entry == XA_ZERO_ENTRY);
}

static inline bool xas_top(struct xa_node *node)
{
    return node <= XAS_RESTART;
}

static inline struct xa_node *xa_to_node(const void *entry)
{
    return (struct xa_node *)((unsigned long)entry - 2);
}

static inline void *xa_mk_node(const struct xa_node *node)
{
    return (void *)((unsigned long)node | 2);
}

void *xas_load(struct xa_state *xas);

void *xas_store(struct xa_state *xas, void *entry);

bool xas_nomem(struct xa_state *xas, gfp_t gfp);

static inline void xas_set_err(struct xa_state *xas, long err)
{
    xas->xa_node = XA_ERROR(err);
}

void *xa_load(struct xarray *xa, unsigned long index);

#endif /* _LINUX_XARRAY_H */
