/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BIO_H
#define __LINUX_BIO_H

#include <gfp.h>
#include <mempool.h>
#include <blk_types.h>

#define BIO_POOL_SIZE 2

enum {
    BIOSET_NEED_BVECS = BIT(0),
    BIOSET_NEED_RESCUER = BIT(1),
};

struct bio_set {
    struct kmem_cache *bio_slab;

    mempool_t bio_pool;
};

struct bio *
bio_alloc_bioset(gfp_t gfp_mask,
                 unsigned int nr_iovecs,
                 struct bio_set *bs);

extern struct bio_set fs_bio_set;

static inline struct bio *
bio_alloc(gfp_t gfp_mask, unsigned int nr_iovecs)
{
    return bio_alloc_bioset(gfp_mask, nr_iovecs, &fs_bio_set);
}

#endif /* __LINUX_BIO_H */
