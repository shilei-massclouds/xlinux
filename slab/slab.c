// SPDX-License-Identifier: GPL-2.0-only
#include <bug.h>
#include <list.h>
#include <log2.h>
#include <slab.h>
#include <export.h>
#include <kernel.h>
#include <string.h>
#include <percpu.h>
#include <printk.h>

#define CFLGS_OBJFREELIST_SLAB  ((slab_flags_t)0x40000000U)
#define OBJFREELIST_SLAB(x) ((x)->flags & CFLGS_OBJFREELIST_SLAB)

#define BATCHREFILL_LIMIT   16

typedef unsigned short freelist_idx_t;

struct array_cache {
    unsigned int avail;
    unsigned int limit;
    unsigned int batchcount;
    unsigned int touched;
    void *entry[];
    /*
     * Must have this definition in here for the proper
     * alignment of array_cache. Also simplifies accessing
     * the entries.
     */
};

struct kmem_cache *
kmalloc_caches[NR_KMALLOC_TYPES][KMALLOC_SHIFT_HIGH + 1] = {};
EXPORT_SYMBOL(kmalloc_caches);

/*
 * Conversion table for small slabs sizes / 8 to the index in the
 * kmalloc array. This is necessary for slabs < 192 since we have
 * non power of two cache sizes there. The size of larger slabs can
 * be determined using fls.
 */
static u8
size_index[24] = {
    3,  /* 8 */
    4,  /* 16 */
    5,  /* 24 */
    5,  /* 32 */
    6,  /* 40 */
    6,  /* 48 */
    6,  /* 56 */
    6,  /* 64 */
    1,  /* 72 */
    1,  /* 80 */
    1,  /* 88 */
    1,  /* 96 */
    7,  /* 104 */
    7,  /* 112 */
    7,  /* 120 */
    7,  /* 128 */
    2,  /* 136 */
    2,  /* 144 */
    2,  /* 152 */
    2,  /* 160 */
    2,  /* 168 */
    2,  /* 176 */
    2,  /* 184 */
    2   /* 192 */
};

static inline unsigned int
size_index_elem(unsigned int bytes)
{
    return (bytes - 1) / 8;
}

struct kmem_cache *
kmalloc_slab(size_t size, gfp_t flags)
{
    unsigned int index;

    if (size <= 192) {
        if (!size)
            return ZERO_SIZE_PTR;

        index = size_index[size_index_elem(size)];
    } else {
        BUG_ON(size > KMALLOC_MAX_CACHE_SIZE);
        index = fls(size - 1);
    }

    return kmalloc_caches[kmalloc_type(flags)][index];
}

static inline struct array_cache *
cpu_cache_get(struct kmem_cache *cachep)
{
    return this_cpu_ptr(cachep->cpu_cache);
}

static int
transfer_objects(struct array_cache *to,
                 struct array_cache *from,
                 unsigned int max)
{
    /* Figure out how many entries to transfer */
    int nr = min3(from->avail, max, to->limit - to->avail);

    if (!nr)
        return 0;

    memcpy(to->entry + to->avail,
           from->entry + from->avail - nr,
           sizeof(void *) * nr);

    from->avail -= nr;
    to->avail += nr;
    return nr;
}

static struct page *
get_first_slab(struct kmem_cache_node *n, bool pfmemalloc)
{
    struct page *page;

    page = list_first_entry_or_null(&n->slabs_partial,
                                    struct page, slab_list);
    if (!page) {
        page = list_first_entry_or_null(&n->slabs_free,
                                        struct page, slab_list);
        if (page)
            n->free_slabs--;
    }

    return page;
}

static inline void *
index_to_obj(struct kmem_cache *cache,
             struct page *page,
             unsigned int idx)
{
    return page->s_mem + cache->size * idx;
}

static inline freelist_idx_t
get_free_obj(struct page *page, unsigned int idx)
{
    return ((freelist_idx_t *)page->freelist)[idx];
}

static void *
slab_get_obj(struct kmem_cache *cachep, struct page *page)
{
    void *objp;

    objp = index_to_obj(cachep, page, get_free_obj(page, page->active));
    page->active++;

    return objp;
}

static __always_inline int
alloc_block(struct kmem_cache *cachep,
            struct array_cache *ac,
            struct page *page,
            int batchcount)
{
    /*
     * There must be at least one object available for
     * allocation.
     */
    BUG_ON(page->active >= cachep->num);

    while (page->active < cachep->num && batchcount--) {
        ac->entry[ac->avail++] = slab_get_obj(cachep, page);
    }

    return batchcount;
}

static inline void
fixup_slab_list(struct kmem_cache *cachep,
                struct kmem_cache_node *n, struct page *page,
                void **list)
{
    /* move slabp to correct slabp list: */
    list_del(&page->slab_list);
    if (page->active == cachep->num) {
        list_add(&page->slab_list, &n->slabs_full);
        if (OBJFREELIST_SLAB(cachep))
            page->freelist = NULL;
    } else {
        list_add(&page->slab_list, &n->slabs_partial);
    }
}

static inline gfp_t
gfp_exact_node(gfp_t flags)
{
    return flags & ~__GFP_NOFAIL;
}

static struct page *
cache_grow_begin(struct kmem_cache *cachep,
                 gfp_t flags,
                 int nodeid)
{
    panic("%s: ...\n", __func__);
    return NULL;
}

static void
cache_grow_end(struct kmem_cache *cachep, struct page *page)
{
    panic("%s: ...\n", __func__);
}

static void *
cache_alloc_refill(struct kmem_cache *cachep, gfp_t flags)
{
    int batchcount;
    struct array_cache *ac;
    struct array_cache *shared;
    struct kmem_cache_node *n;
    struct page *page;
    void *list = NULL;

    ac = cpu_cache_get(cachep);
    batchcount = ac->batchcount;
    if (!ac->touched && batchcount > BATCHREFILL_LIMIT) {
        /*
         * If there was little recent activity on this cache, then
         * perform only a partial refill.  Otherwise we could generate
         * refill bouncing.
         */
        batchcount = BATCHREFILL_LIMIT;
    }
    n = get_node(cachep, 0);

    BUG_ON(ac->avail > 0 || !n);
    shared = n->shared;
    if (!n->free_objects && (!shared || !shared->avail))
        goto direct_grow;

    shared = n->shared;

    /* See if we can refill from the shared array */
    if (shared && transfer_objects(ac, shared, batchcount)) {
        shared->touched = 1;
        goto direct_grow;
    }

    while (batchcount > 0) {
        /* Get slab alloc is to come from. */
        page = get_first_slab(n, false);
        if (!page)
            goto must_grow;

        batchcount = alloc_block(cachep, ac, page, batchcount);
        fixup_slab_list(cachep, n, page, &list);
    }

 must_grow:
    n->free_objects -= ac->avail;

 direct_grow:
    if (unlikely(!ac->avail)) {
        page = cache_grow_begin(cachep, gfp_exact_node(flags), 0);

        /*
         * cache_grow_begin() can reenable interrupts,
         * then ac could change.
         */
        ac = cpu_cache_get(cachep);
        if (!ac->avail && page)
            alloc_block(cachep, ac, page, batchcount);
        cache_grow_end(cachep, page);

        if (!ac->avail)
            return NULL;
    }

    ac->touched = 1;
    return ac->entry[--ac->avail];
}

static inline void *
____cache_alloc(struct kmem_cache *cachep, gfp_t flags)
{
    struct array_cache *ac;
    ac = cpu_cache_get(cachep);
    if (likely(ac->avail)) {
        ac->touched = 1;
        return ac->entry[--ac->avail];
    }
    return cache_alloc_refill(cachep, flags);
}

static __always_inline void *
__do_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
{
    return ____cache_alloc(cachep, flags);
}

static inline bool
slab_want_init_on_alloc(gfp_t flags, struct kmem_cache *c)
{
    return flags & __GFP_ZERO;
}

static __always_inline void *
slab_alloc(struct kmem_cache *cachep, gfp_t flags, unsigned long caller)
{
    void *objp;
    objp = __do_cache_alloc(cachep, flags);

    if (unlikely(slab_want_init_on_alloc(flags, cachep)) && objp)
        memset(objp, 0, cachep->object_size);

    return objp;
}

static __always_inline void *
__do_kmalloc(size_t size, gfp_t flags, unsigned long caller)
{
    struct kmem_cache *cachep;
    void *ret;

    if (unlikely(size > KMALLOC_MAX_CACHE_SIZE))
        return NULL;

    cachep = kmalloc_slab(size, flags);
    if (unlikely(ZERO_OR_NULL_PTR(cachep)))
        return cachep;

    return slab_alloc(cachep, flags, caller);
}

void *
__kmalloc(size_t size, gfp_t flags)
{
    return __do_kmalloc(size, flags, _RET_IP_);
}
EXPORT_SYMBOL(__kmalloc);
