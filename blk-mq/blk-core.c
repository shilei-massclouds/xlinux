// SPDX-License-Identifier: GPL-2.0

#include <slab.h>
#include <blkdev.h>

struct kmem_cache *blk_requestq_cachep;

struct request_queue *
blk_alloc_queue()
{
    int ret;
    struct request_queue *q;

    q = kmem_cache_alloc(blk_requestq_cachep, GFP_KERNEL|__GFP_ZERO);
    if (!q)
        return NULL;

    return q;
}

int
blk_dev_init(void)
{
    blk_requestq_cachep =
        kmem_cache_create("request_queue",
                          sizeof(struct request_queue),
                          0, SLAB_PANIC, NULL);

    return 0;
}
