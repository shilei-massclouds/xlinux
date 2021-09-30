/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SLAB_H
#define _LINUX_SLAB_H

#include <gfp.h>
#include <types.h>
#include <page.h>
#include <kernel.h>
#include <compiler_attributes.h>

#define KMALLOC_SHIFT_HIGH  \
    ((MAX_ORDER + PAGE_SHIFT - 1) <= 25 ? (MAX_ORDER + PAGE_SHIFT - 1) : 25)
#define KMALLOC_SHIFT_MAX   KMALLOC_SHIFT_HIGH
#define KMALLOC_SHIFT_LOW   5

/* Maximum allocatable size */
#define KMALLOC_MAX_SIZE        (1UL << KMALLOC_SHIFT_MAX)
/* Maximum size for which we actually use a slab cache */
#define KMALLOC_MAX_CACHE_SIZE  (1UL << KMALLOC_SHIFT_HIGH)
/* Maximum order allocatable via the slab allocagtor */
#define KMALLOC_MAX_ORDER       (KMALLOC_SHIFT_MAX - PAGE_SHIFT)

/*
 * ZERO_SIZE_PTR will be returned for zero sized kmalloc requests.
 *
 * Dereferencing ZERO_SIZE_PTR will lead to a distinct access fault.
 *
 * ZERO_SIZE_PTR can be passed to kfree though in the same way that NULL can.
 * Both make kfree a no-op.
 */
#define ZERO_SIZE_PTR ((void *)16)

#define ZERO_OR_NULL_PTR(x) ((unsigned long)(x) <= (unsigned long)ZERO_SIZE_PTR)

#define KMALLOC_MIN_SIZE (1 << KMALLOC_SHIFT_LOW)

#define SLAB_OBJ_MIN_SIZE \
    (KMALLOC_MIN_SIZE < 16 ? (KMALLOC_MIN_SIZE) : 16)

#define SLAB_HWCACHE_ALIGN      ((slab_flags_t)0x00002000U)
#define ARCH_SLAB_MINALIGN      __alignof__(unsigned long long)
#define ARCH_KMALLOC_MINALIGN   __alignof__(unsigned long long)

enum slab_state {
    DOWN,           /* No slab functionality yet */
    PARTIAL,        /* SLUB: kmem_cache_node available */
    PARTIAL_NODE,   /* SLAB: kmalloc size for node struct available */
    UP,             /* Slab caches usable but not all extras yet */
    FULL            /* Everything is working */
};

enum kmalloc_cache_type {
    KMALLOC_NORMAL = 0,
    KMALLOC_RECLAIM,
    NR_KMALLOC_TYPES
};

struct kmem_cache_node {
    struct list_head slabs_partial; /* partial list first, better asm code */
    struct list_head slabs_full;
    struct list_head slabs_free;
    unsigned long total_slabs;      /* length of all slab lists */
    unsigned long free_slabs;       /* length of free slab list only */
    unsigned long free_objects;
//    unsigned int free_limit;
    unsigned int colour_next;       /* Per-node cache coloring */
    struct array_cache *shared;     /* shared per node */
    struct alien_cache **alien;     /* on other nodes */
//    unsigned long next_reap;    /* updated without locking */
    int free_touched;               /* updated without locking */
};

struct kmem_cache {
    struct array_cache *cpu_cache;

/* 1) Cache tunables. */
    unsigned int batchcount;
    unsigned int limit;
    unsigned int shared;

    unsigned int size;

/* 2) touched by every alloc & free from the backend */
    slab_flags_t flags; /* constant flags */
    unsigned int num;   /* # of objs per slab */

/* 4) cache creation/removal */
    const char *name;
    struct list_head list;
    int object_size;
    int align;

    struct kmem_cache_node *node[MAX_NUMNODES];
};

void *__kmalloc(size_t size, gfp_t flags);

void kmem_cache_init(void);

static __always_inline void *
kmalloc(size_t size, gfp_t flags)
{
    return __kmalloc(size, flags);
}

static __always_inline void *
__kmalloc_node(size_t size, gfp_t flags, int node)
{
    return __kmalloc(size, flags);
}

static __always_inline void *
kmalloc_node(size_t size, gfp_t flags, int node)
{
    return __kmalloc_node(size, flags, node);
}

static inline void *
kzalloc(size_t size, gfp_t flags)
{
    return kmalloc(size, flags | __GFP_ZERO);
}

static __always_inline enum kmalloc_cache_type
kmalloc_type(gfp_t flags)
{
    return flags & __GFP_RECLAIMABLE ? KMALLOC_RECLAIM : KMALLOC_NORMAL;
}

static inline struct kmem_cache_node *
get_node(struct kmem_cache *s, int node)
{
    return s->node[node];
}

#endif /* _LINUX_SLAB_H */
