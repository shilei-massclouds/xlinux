/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MEMPOOL_H
#define _LINUX_MEMPOOL_H

typedef struct mempool_s {
} mempool_t;

static inline int
mempool_init_slab_pool(mempool_t *pool, int min_nr, struct kmem_cache *kc)
{
    panic("%s: !", __func__);
    /*
    return mempool_init(pool, min_nr, mempool_alloc_slab,
                        mempool_free_slab, (void *) kc);
                        */
}

#endif /* _LINUX_MEMPOOL_H */
