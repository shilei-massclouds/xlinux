/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BIO_H
#define __LINUX_BIO_H

#include <gfp.h>
#include <blk_types.h>

#define BIO_POOL_SIZE 2

enum {
    BIOSET_NEED_BVECS = BIT(0),
    BIOSET_NEED_RESCUER = BIT(1),
};

struct bio_set {
    struct kmem_cache *bio_slab;
    unsigned int front_pad;
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

#define bio_set_dev(bio, bdev)              \
do {                                        \
    if ((bio)->bi_disk != (bdev)->bd_disk)  \
        bio_clear_flag(bio, BIO_THROTTLED); \
    (bio)->bi_disk = (bdev)->bd_disk;       \
    (bio)->bi_partno = (bdev)->bd_partno;   \
} while (0)

static inline void bio_clear_flag(struct bio *bio, unsigned int bit)
{
    bio->bi_flags &= ~(1U << bit);
}

#endif /* __LINUX_BIO_H */
