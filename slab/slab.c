// SPDX-License-Identifier: GPL-2.0-only
//#define X_DEBUG
#include <mm.h>
#include <errno.h>
#include <list.h>
#include <log2.h>
#include <slab.h>
#include <export.h>
#include <kernel.h>
#include <string.h>
#include <printk.h>

#define BYTES_PER_WORD  sizeof(void *)

#define CFLGS_OBJFREELIST_SLAB  ((slab_flags_t)0x40000000U)
#define OBJFREELIST_SLAB(x) ((x)->flags & CFLGS_OBJFREELIST_SLAB)

#define CACHE_CACHE 0
#define SIZE_NODE (MAX_NUMNODES)

#define BATCHREFILL_LIMIT   16

#define ARCH_KMALLOC_FLAGS SLAB_HWCACHE_ALIGN

#define INDEX_NODE kmalloc_index(sizeof(struct kmem_cache_node))

#define SLAB_OBJ_MAX_NUM \
    ((1 << sizeof(freelist_idx_t) * BITS_PER_BYTE) - 1)

#define MAKE_LIST(cachep, listp, slab, nodeid)  \
    do {                                        \
        INIT_LIST_HEAD(listp);                  \
        list_splice(&get_node(cachep, nodeid)->slab, listp);    \
    } while (0)

#define MAKE_ALL_LISTS(cachep, ptr, nodeid) \
    do {                                    \
    MAKE_LIST((cachep), (&(ptr)->slabs_full), slabs_full, nodeid); \
    MAKE_LIST((cachep), (&(ptr)->slabs_partial), slabs_partial, nodeid); \
    MAKE_LIST((cachep), (&(ptr)->slabs_free), slabs_free, nodeid);  \
    } while (0)

#define FREELIST_BYTE_INDEX \
    (((PAGE_SIZE >> BITS_PER_BYTE) <= SLAB_OBJ_MIN_SIZE) ? 1 : 0)

#if FREELIST_BYTE_INDEX
typedef unsigned char freelist_idx_t;
#else
typedef unsigned short freelist_idx_t;
#endif

/*
 * Do not go above this order unless 0 objects fit into the slab or
 * overridden on the command line.
 */
#define SLAB_MAX_ORDER_HI   1
#define SLAB_MAX_ORDER_LO   0
static int slab_max_order = SLAB_MAX_ORDER_LO;

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

extern size_t reserved_chunk_size;
extern void *reserved_chunk_ptr;

struct kmem_cache *
kmalloc_caches[NR_KMALLOC_TYPES][KMALLOC_SHIFT_HIGH + 1] = {};
EXPORT_SYMBOL(kmalloc_caches);

LIST_HEAD(slab_caches);

#define NUM_INIT_LISTS (2 * MAX_NUMNODES)
static struct kmem_cache_node init_kmem_cache_node[NUM_INIT_LISTS];

enum slab_state slab_state;
struct kmem_cache *kmem_cache;

#define BOOT_CPUCACHE_ENTRIES   1
/* internal cache of cache description objs */
static struct kmem_cache kmem_cache_boot = {
    .batchcount = 1,
    .limit = BOOT_CPUCACHE_ENTRIES,
    .shared = 1,
    .size = sizeof(struct kmem_cache),
    .name = "kmem_cache",
};

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

#define INIT_KMALLOC_INFO(__size, __short_size)             \
{                                                           \
    .name[KMALLOC_NORMAL]  = "kmalloc-" #__short_size,      \
    .name[KMALLOC_RECLAIM] = "kmalloc-rcl-" #__short_size,  \
    .size = __size,                     \
}

const struct kmalloc_info_struct kmalloc_info[] = {
    INIT_KMALLOC_INFO(0, 0),
    INIT_KMALLOC_INFO(96, 96),
    INIT_KMALLOC_INFO(192, 192),
    INIT_KMALLOC_INFO(8, 8),
    INIT_KMALLOC_INFO(16, 16),
    INIT_KMALLOC_INFO(32, 32),
    INIT_KMALLOC_INFO(64, 64),
    INIT_KMALLOC_INFO(128, 128),
    INIT_KMALLOC_INFO(256, 256),
    INIT_KMALLOC_INFO(512, 512),
    INIT_KMALLOC_INFO(1024, 1k),
    INIT_KMALLOC_INFO(2048, 2k),
    INIT_KMALLOC_INFO(4096, 4k),
    INIT_KMALLOC_INFO(8192, 8k),
    INIT_KMALLOC_INFO(16384, 16k),
    INIT_KMALLOC_INFO(32768, 32k),
    INIT_KMALLOC_INFO(65536, 64k),
    INIT_KMALLOC_INFO(131072, 128k),
    INIT_KMALLOC_INFO(262144, 256k),
    INIT_KMALLOC_INFO(524288, 512k),
    INIT_KMALLOC_INFO(1048576, 1M),
    INIT_KMALLOC_INFO(2097152, 2M),
    INIT_KMALLOC_INFO(4194304, 4M),
    INIT_KMALLOC_INFO(8388608, 8M),
    INIT_KMALLOC_INFO(16777216, 16M),
    INIT_KMALLOC_INFO(33554432, 32M),
    INIT_KMALLOC_INFO(67108864, 64M)
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

    pr_debug("%s: type(%u) index(%u)\n",
           __func__, kmalloc_type(flags), index);
    return kmalloc_caches[kmalloc_type(flags)][index];
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
    pr_debug("%s: s_mem(%lx) idx(%u) size(%u)\n",
           __func__, page->s_mem, idx, cache->size);
    return page->s_mem + cache->size * idx;
}

static inline freelist_idx_t
get_free_obj(struct page *page, unsigned int idx)
{
    BUG_ON(page->freelist == NULL);
    return ((freelist_idx_t *)page->freelist)[idx];
}

static void *
slab_get_obj(struct kmem_cache *cachep, struct page *page)
{
    void *objp;
    objp = index_to_obj(cachep, page, get_free_obj(page, page->active));
    pr_debug("%s: objp(%lx)\n", __func__, objp);
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

    pr_debug("%s: ... active(%u) num(%u) freelist(%lx)\n",
           __func__, page->active, cachep->num, page->freelist);

    while (page->active < cachep->num && batchcount--) {
        ac->entry[ac->avail++] = slab_get_obj(cachep, page);
    }
    pr_debug("%s: ok! avail(%u) active(%u)\n", __func__, ac->avail, page->active);

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
        pr_debug("+++ %s: page(%lx)\n", __func__, page);
        list_add(&page->slab_list, &n->slabs_partial);
    }
}

static inline gfp_t
gfp_exact_node(gfp_t flags)
{
    return flags & ~__GFP_NOFAIL;
}

/*
 * Interface to system's page allocator. No need to hold the
 * kmem_cache_node ->list_lock.
 *
 * If we requested dmaable memory, we will get it. Even if we
 * did not request dmaable memory, we might get it, but that
 * would be relatively rare and ignorable.
 */
static struct page *
kmem_getpages(struct kmem_cache *cachep, gfp_t flags, int nodeid)
{
    struct page *page;

    page = __alloc_pages_node(nodeid, flags, cachep->gfporder);
    if (!page) {
        panic("slab out of memory!\n");
        return NULL;
    }

    return page;
}

static void *
alloc_slabmgmt(struct kmem_cache *cachep,
               struct page *page, int colour_off,
               gfp_t local_flags, int nodeid)
{
    void *freelist;
    void *addr = page_address(page);

    pr_debug("%s: addr(%lx) colour_off(%d)\n",
           __func__, addr, colour_off);

    page->s_mem = addr + colour_off;
    page->active = 0;

    if (OBJFREELIST_SLAB(cachep)) {
        pr_debug("%s: freelist_size NULL!\n", __func__);
        freelist = NULL;
    } else {
        /* We will use last bytes at the slab for freelist */
        freelist = addr +
            (PAGE_SIZE << cachep->gfporder) - cachep->freelist_size;

        pr_debug("%s: freelist_size(%u)\n", __func__, cachep->freelist_size);
    }

    return freelist;
}

static void
slab_map_pages(struct kmem_cache *cache,
               struct page *page,
               void *freelist)
{
    page->slab_cache = cache;
    page->freelist = freelist;
}

static inline void
set_free_obj(struct page *page, unsigned int idx, freelist_idx_t val)
{
    ((freelist_idx_t *)(page->freelist))[idx] = val;
}

static void
cache_init_objs(struct kmem_cache *cachep, struct page *page)
{
    int i;

    if (OBJFREELIST_SLAB(cachep)) {
        page->freelist = index_to_obj(cachep, page, cachep->num - 1);
    }

    for (i = 0; i < cachep->num; i++)
        set_free_obj(page, i, i);
}

static struct page *
cache_grow_begin(struct kmem_cache *cachep, gfp_t flags, int nodeid)
{
    void *freelist;
    size_t offset;
    struct page *page;

    pr_debug("%s: ...\n", __func__);

    offset = 0;

    /*
     * Get mem for the objs.  Attempt to allocate a physical page from
     * 'nodeid'.
     */
    page = kmem_getpages(cachep, flags, nodeid);
    if (!page)
        goto failed;

    /* Get slab management. */
    freelist = alloc_slabmgmt(cachep, page, offset, flags, 0);

    slab_map_pages(cachep, page, freelist);

    cache_init_objs(cachep, page);

    pr_debug("%s: ok! freelist(%lx)\n", __func__, page->freelist);

    return page;

 failed:
    panic("%s: ... (%u)(%d)\n", __func__, flags, nodeid);
    return NULL;
}

static void
cache_grow_end(struct kmem_cache *cachep, struct page *page)
{
    struct kmem_cache_node *n;
    void *list = NULL;

    pr_debug("%s: ... page->active(%u)\n", __func__, page->active);

    if (!page)
        return;

    INIT_LIST_HEAD(&page->slab_list);
    n = get_node(cachep, 0);

    n->total_slabs++;
    if (!page->active) {
        list_add_tail(&page->slab_list, &n->slabs_free);
        n->free_slabs++;
    } else {
        fixup_slab_list(cachep, n, page, &list);
    }

    n->free_objects += cachep->num - page->active;

    pr_debug("%s: ok!\n", __func__);
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

    pr_debug("%s: flags(%x)\n", __func__, flags);

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
    if (!n->free_objects && (!shared || !shared->avail)) {
        pr_debug("%s: free_objects zero\n", __func__);
        goto direct_grow;
    }

    pr_debug("%s: free_objects NOT zero! free_objects(%u)\n",
           __func__, n->free_objects);

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

    pr_debug("%s: ok! avail(%u) ret(%lx)\n",
           __func__, ac->avail, ac->entry[ac->avail-1]);
    return ac->entry[--ac->avail];
}

static inline void *
____cache_alloc(struct kmem_cache *cachep, gfp_t flags)
{
    struct array_cache *ac;
    ac = cpu_cache_get(cachep);
    if (likely(ac->avail)) {
        pr_debug("%s: avail(%u)\n", __func__, ac->avail);
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

    pr_debug("%s: (%lx) ...\n", __func__, cachep->object_size);

    objp = __do_cache_alloc(cachep, flags);

    if (unlikely(slab_want_init_on_alloc(flags, cachep)) && objp)
        memset(objp, 0, cachep->object_size);

    pr_debug("%s: (%lx) (%lx) ok!\n",
           __func__, cachep->object_size, objp);

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
    pr_debug("%s: (%lx)(%lx)(%lx) %u\n",
           __func__, cachep, cpu_cache_get(cachep),
           cachep->object_size, size);
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

static void
kmem_cache_node_init(struct kmem_cache_node *parent)
{
    INIT_LIST_HEAD(&parent->slabs_full);
    INIT_LIST_HEAD(&parent->slabs_partial);
    INIT_LIST_HEAD(&parent->slabs_free);
    parent->total_slabs = 0;
    parent->free_slabs = 0;
    parent->shared = NULL;
    parent->alien = NULL;
    parent->colour_next = 0;
    parent->free_objects = 0;
    parent->free_touched = 0;
}

static unsigned int
calculate_alignment(slab_flags_t flags,
                    unsigned int align, unsigned int size)
{
    /*
     * If the user wants hardware cache aligned objects then follow that
     * suggestion if the object is sufficiently large.
     *
     * The hardware cache alignment cannot override the specified
     * alignment though. If that is greater then use it.
     */
    if (flags & SLAB_HWCACHE_ALIGN) {
        unsigned int ralign;

        ralign = cache_line_size();
        while (size <= ralign / 2)
            ralign /= 2;
        align = max(align, ralign);
    }

    if (align < ARCH_SLAB_MINALIGN)
        align = ARCH_SLAB_MINALIGN;

    return ALIGN(align, sizeof(void *));
}

bool slab_is_available(void)
{
    return slab_state >= UP;
}

/* Called with slab_mutex held always */
static int
enable_cpucache(struct kmem_cache *cachep, gfp_t gfp)
{
    int err;
    int limit = 0;
    int shared = 0;
    int batchcount = 0;
}

static void
init_arraycache(struct array_cache *ac, int limit, int batch)
{
    if (ac) {
        ac->avail = 0;
        ac->limit = limit;
        ac->batchcount = batch;
        ac->touched = 0;
    }
}

static struct array_cache *
alloc_kmem_cache_cpus(struct kmem_cache *cachep,
                      int entries,
                      int batchcount)
{
    int cpu;
    size_t size;
    struct array_cache *cpu_cache;

    pr_debug("%s: (%lx) object_size(%lx) ...\n",
             __func__, cachep, cachep->object_size);

    size = sizeof(void *) * entries + sizeof(struct array_cache);
    if (size > reserved_chunk_size || !reserved_chunk_ptr)
        panic("bad reserved chunk [%lx] (%lu)",
              reserved_chunk_ptr, reserved_chunk_size);
    cpu_cache = reserved_chunk_ptr;
    if (!cpu_cache)
        return NULL;

    init_arraycache(cpu_cache, entries, batchcount);

    pr_debug("%s: ok! (%lx)\n", __func__, cpu_cache_get(cachep));

    return cpu_cache;
}

static void
set_up_node(struct kmem_cache *cachep, int index)
{
    cachep->node[0] = &init_kmem_cache_node[index];
}

static int
setup_cpu_cache(struct kmem_cache *cachep, gfp_t gfp)
{
    pr_debug("%s: ... slab_state(%u)\n", __func__, slab_state);

    if (slab_state >= FULL)
        return enable_cpucache(cachep, gfp);

    cachep->cpu_cache = alloc_kmem_cache_cpus(cachep, 1, 1);
    if (!cachep->cpu_cache)
        return 1;

    if (slab_state == DOWN) {
        /* Creation of first cache (kmem_cache). */
        set_up_node(kmem_cache, CACHE_CACHE);
    } else if (slab_state == PARTIAL) {
        /* For kmem_cache_node */
        set_up_node(cachep, SIZE_NODE);
    } else {
        cachep->node[0] = kmalloc_node(
            sizeof(struct kmem_cache_node), gfp, 0);
        BUG_ON(!cachep->node[0]);
        kmem_cache_node_init(cachep->node[0]);
    }

    cpu_cache_get(cachep)->avail = 0;
    cpu_cache_get(cachep)->limit = BOOT_CPUCACHE_ENTRIES;
    cpu_cache_get(cachep)->batchcount = 1;
    cpu_cache_get(cachep)->touched = 0;
    cachep->batchcount = 1;
    cachep->limit = BOOT_CPUCACHE_ENTRIES;

    pr_debug("%s: ok! (%lx)(%lx)\n", __func__, cachep, cpu_cache_get(cachep));
    return 0;
}

void
__kmem_cache_release(struct kmem_cache *cachep)
{
    // Todo:
}

static unsigned int
cache_estimate(unsigned long gfporder,
               size_t buffer_size,
               slab_flags_t flags,
               size_t *left_over)
{
    unsigned int num;
    size_t slab_size = PAGE_SIZE << gfporder;

    if (flags & (CFLGS_OBJFREELIST_SLAB)) {
        num = slab_size / buffer_size;
        *left_over = slab_size % buffer_size;
    } else {
        num = slab_size / (buffer_size + sizeof(freelist_idx_t));
        *left_over = slab_size %
            (buffer_size + sizeof(freelist_idx_t));
    }

    return num;
}

static size_t
calculate_slab_order(struct kmem_cache *cachep,
                     size_t size,
                     slab_flags_t flags)
{
    size_t left_over = 0;
    int gfporder;

    for (gfporder = 0; gfporder <= KMALLOC_MAX_ORDER; gfporder++) {
        unsigned int num;
        size_t remainder;

        num = cache_estimate(gfporder, size, flags, &remainder);
        if (!num)
            continue;

        /* Can't handle number of objects more than SLAB_OBJ_MAX_NUM */
        if (num > SLAB_OBJ_MAX_NUM)
            break;

        /* Found something acceptable - save it away */
        cachep->num = num;
        cachep->gfporder = gfporder;
        left_over = remainder;

        /*
         * Large number of objects is good, but very large slabs are
         * currently bad for the gfp()s.
         */
        if (gfporder >= slab_max_order)
            break;

        /*
         * Acceptable internal fragmentation?
         */
        if (left_over * 8 <= (PAGE_SIZE << gfporder))
            break;
    }
    return left_over;
}

static bool
set_objfreelist_slab_cache(struct kmem_cache *cachep,
                           size_t size,
                           slab_flags_t flags)
{
    size_t left;

    cachep->num = 0;

    left = calculate_slab_order(cachep, size,
                                flags | CFLGS_OBJFREELIST_SLAB);
    if (!cachep->num)
        return false;

    if (cachep->num * sizeof(freelist_idx_t) > cachep->object_size)
        return false;

    cachep->colour = left / cachep->colour_off;
    return true;
}

static bool
set_on_slab_cache(struct kmem_cache *cachep, size_t size, slab_flags_t flags)
{
    size_t left;

    cachep->num = 0;

    left = calculate_slab_order(cachep, size, flags);
    if (!cachep->num)
        return false;

    cachep->colour = left / cachep->colour_off;

    return true;
}

int
__kmem_cache_create(struct kmem_cache *cachep, slab_flags_t flags)
{
    size_t ralign = BYTES_PER_WORD;
    gfp_t gfp;
    int err;
    unsigned int size = cachep->size;

    pr_debug("%s: ... size(%u) flags(%x)\n", __func__, size, flags);

    /*
     * Check that size is in terms of words.  This is needed to avoid
     * unaligned accesses for some archs when redzoning is used, and makes
     * sure any on-slab bufctl's are also correctly aligned.
     */
    size = ALIGN(size, BYTES_PER_WORD);

    if (ralign < cachep->align) {
        ralign = cachep->align;
    }

    cachep->align = ralign;
    cachep->colour_off = cache_line_size();
    /* Offset must be a multiple of the alignment. */
    if (cachep->colour_off < cachep->align)
        cachep->colour_off = cachep->align;

    if (slab_is_available())
        gfp = GFP_KERNEL;
    else
        gfp = GFP_NOWAIT;

    size = ALIGN(size, cachep->align);

    if (FREELIST_BYTE_INDEX && size < SLAB_OBJ_MIN_SIZE)
        size = ALIGN(SLAB_OBJ_MIN_SIZE, cachep->align);

    pr_debug("%s: colour_off(%u) size(%u) align(%u)\n",
           __func__, cachep->colour_off, size, cachep->align);

    if (set_objfreelist_slab_cache(cachep, size, flags)) {
        pr_debug("%s: carry objfreelist ...\n", __func__);
        flags |= CFLGS_OBJFREELIST_SLAB;
        goto done;
    }

    if (set_on_slab_cache(cachep, size, flags))
        goto done;

    panic("bad cache!\n");
    return -E2BIG;

 done:
    cachep->freelist_size = cachep->num * sizeof(freelist_idx_t);
    cachep->flags = flags;
    cachep->size = size;

    err = setup_cpu_cache(cachep, gfp);
    if (err) {
        __kmem_cache_release(cachep);
        return err;
    }

    pr_debug("%s: ok!\n", __func__);

    return 0;
}

/* Create a cache during boot when no slab services are available yet */
void
create_boot_cache(struct kmem_cache *s, const char *name,
                  unsigned int size, slab_flags_t flags,
                  unsigned int useroffset, unsigned int usersize)
{
    int err;
    unsigned int align = ARCH_KMALLOC_MINALIGN;

    s->name = name;
    s->size = s->object_size = size;

    /*
     * For power of two sizes, guarantee natural alignment for kmalloc
     * caches, regardless of SL*B debugging options.
     */
    if (is_power_of_2(size))
        align = max(align, size);
    s->align = calculate_alignment(flags, align, size);

    s->useroffset = useroffset;
    s->usersize = usersize;

    err = __kmem_cache_create(s, flags);
    if (err)
        panic("Creation of kmalloc slab %s size=%u failed. Reason %d\n",
              name, size, err);

    s->refcount = -1;
}

struct kmem_cache *
create_kmalloc_cache(const char *name,
                     unsigned int size, slab_flags_t flags,
                     unsigned int useroffset, unsigned int usersize)
{
    struct kmem_cache *s = kmem_cache_zalloc(kmem_cache, GFP_NOWAIT);
    if (!s)
        panic("Out of memory when creating slab %s\n", name);

    create_boot_cache(s, name, size, flags, useroffset, usersize);
    list_add(&s->list, &slab_caches);
    s->refcount = 1;
    return s;
}

void *
kmem_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
{
    return slab_alloc(cachep, flags, _RET_IP_);
}
EXPORT_SYMBOL(kmem_cache_alloc);

void setup_kmalloc_cache_index_table(void)
{
    unsigned int i;

    BUG_ON(KMALLOC_MIN_SIZE > 256 ||
           (KMALLOC_MIN_SIZE & (KMALLOC_MIN_SIZE - 1)));

    for (i = 8; i < KMALLOC_MIN_SIZE; i += 8) {
        unsigned int elem = size_index_elem(i);

        if (elem >= ARRAY_SIZE(size_index))
            break;
        size_index[elem] = KMALLOC_SHIFT_LOW;
    }

    if (KMALLOC_MIN_SIZE >= 64) {
        /*
         * The 96 byte size cache is not used if the alignment
         * is 64 byte.
         */
        for (i = 64 + 8; i <= 96; i += 8)
            size_index[size_index_elem(i)] = 7;

    }

    if (KMALLOC_MIN_SIZE >= 128) {
        /*
         * The 192 byte sized cache is not used if the alignment
         * is 128 byte. Redirect kmalloc to use the 256 byte cache
         * instead.
         */
        for (i = 128 + 8; i <= 192; i += 8)
            size_index[size_index_elem(i)] = 8;
    }
}

/*
 * Initialisation.
 * Called after the page allocator have been initialised and
 * before smp_init().
 */
static void
init_list(struct kmem_cache *cachep,
          struct kmem_cache_node *list,
          int nodeid)
{
    struct kmem_cache_node *ptr;

    ptr = kmalloc_node(sizeof(struct kmem_cache_node), GFP_NOWAIT, nodeid);
    BUG_ON(!ptr);

    memcpy(ptr, list, sizeof(struct kmem_cache_node));

    //MAKE_ALL_LISTS(cachep, ptr, nodeid);
    MAKE_LIST((cachep), (&(ptr)->slabs_full), slabs_full, nodeid);
    MAKE_LIST((cachep), (&(ptr)->slabs_free), slabs_free, nodeid);
    INIT_LIST_HEAD(&ptr->slabs_partial);
    pr_debug("%s: 1 (%lx) cachep(%lx) ptr(%lx)\n",
           __func__, cpu_cache_get(kmalloc_caches[0][7]),
           kmalloc_caches[0][7], ptr);
    {
        struct list_head *one = &ptr->slabs_partial;
        struct list_head *two = &cachep->node[0]->slabs_partial;
        pr_debug("(%lx)\n", two->next);
    }
    list_splice(&cachep->node[0]->slabs_partial, &ptr->slabs_partial);
    pr_debug("%s: 2 (%lx) cachep(%lx) ptr(%lx)\n",
           __func__, cpu_cache_get(kmalloc_caches[0][7]), kmalloc_caches[0][7], ptr);
    cachep->node[nodeid] = ptr;
}

static void
new_kmalloc_cache(int idx, enum kmalloc_cache_type type, slab_flags_t flags)
{
    pr_debug("%s: type(%u) index(%u)\n", __func__, type, idx);
    kmalloc_caches[type][idx] =
        create_kmalloc_cache(kmalloc_info[idx].name[type],
                             kmalloc_info[idx].size, flags, 0,
                             kmalloc_info[idx].size);
}

void
create_kmalloc_caches(slab_flags_t flags)
{
    int i;
    enum kmalloc_cache_type type;

    for (type = KMALLOC_NORMAL; type <= KMALLOC_RECLAIM; type++) {
        for (i = KMALLOC_SHIFT_LOW; i <= KMALLOC_SHIFT_HIGH; i++) {
            pr_debug("%s: type(%d) i(%d)\n", __func__, type, i);
            if (!kmalloc_caches[type][i])
                new_kmalloc_cache(i, type, flags);

            /*
             * Caches that are not of the two-to-the-power-of size.
             * These have to be created immediately after the
             * earlier power of two caches
             */
            if (KMALLOC_MIN_SIZE <= 32 && i == 6 && !kmalloc_caches[type][1])
                new_kmalloc_cache(1, type, flags);
            if (KMALLOC_MIN_SIZE <= 64 && i == 7 && !kmalloc_caches[type][2])
                new_kmalloc_cache(2, type, flags);
        }
    }

    /* Kmalloc array is now usable */
    slab_state = UP;
}

void
kmem_cache_init(void)
{
    int i;

    kmem_cache = &kmem_cache_boot;

    for (i = 0; i < NUM_INIT_LISTS; i++)
        kmem_cache_node_init(&init_kmem_cache_node[i]);

    pr_debug("===%s: init kmem_cache ...\n", __func__);

    create_boot_cache(kmem_cache, "kmem_cache",
                      offsetof(struct kmem_cache, node) +
                      sizeof(struct kmem_cache_node *),
                      SLAB_HWCACHE_ALIGN, 0, 0);
    list_add(&kmem_cache->list, &slab_caches);
    slab_state = PARTIAL;

    pr_debug("===%s: create cache for kmem_cache_node ...\n", __func__);

    /*
     * Initialize the caches that provide memory for the  kmem_cache_node
     * structures first.  Without this, further allocations will bug.
     */
    kmalloc_caches[KMALLOC_NORMAL][INDEX_NODE] =
        create_kmalloc_cache(kmalloc_info[INDEX_NODE].name[KMALLOC_NORMAL],
                             kmalloc_info[INDEX_NODE].size,
                             ARCH_KMALLOC_FLAGS, 0,
                             kmalloc_info[INDEX_NODE].size);

    slab_state = PARTIAL_NODE;
    setup_kmalloc_cache_index_table();

    pr_debug("%s: replace node for kmem_cache ... (%lx)\n",
           __func__, kmalloc_caches[KMALLOC_NORMAL][INDEX_NODE]);

    init_list(kmem_cache, &init_kmem_cache_node[CACHE_CACHE], 0);

    pr_debug("%s: replace node for kmalloc_cache for node size ...\n", __func__);

    init_list(kmalloc_caches[KMALLOC_NORMAL][INDEX_NODE],
              &init_kmem_cache_node[SIZE_NODE], 0);

    pr_debug("%s: replace node for kmalloc_cache for node size ok!\n", __func__);

    pr_debug("=========================\n");
    create_kmalloc_caches(ARCH_KMALLOC_FLAGS);
    pr_debug("=========================\n");

    pr_debug("%s: ok!\n", __func__);
}
EXPORT_SYMBOL(kmem_cache_init);

void *
__kmalloc_track_caller(size_t size, gfp_t flags, unsigned long caller)
{
    return __do_kmalloc(size, flags, caller);
}
EXPORT_SYMBOL(__kmalloc_track_caller);

static int
init_module(void)
{
    printk("module[slab]: init begin ...\n");
    kmem_cache_init();
    printk("module[slab]: init end!\n");
    return 0;
}
