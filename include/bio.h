/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BIO_H
#define __LINUX_BIO_H

#include <gfp.h>
#include <bvec.h>
#include <limits.h>
#include <blk_types.h>

#define BIO_POOL_SIZE 2

#define bio_prio(bio)   (bio)->bi_ioprio

static inline bool bio_no_advance_iter(const struct bio *bio)
{
    return bio_op(bio) == REQ_OP_DISCARD ||
           bio_op(bio) == REQ_OP_SECURE_ERASE ||
           bio_op(bio) == REQ_OP_WRITE_SAME ||
           bio_op(bio) == REQ_OP_WRITE_ZEROES;
}

static inline void
bio_advance_iter(const struct bio *bio,
                 struct bvec_iter *iter,
                 unsigned int bytes)
{
    iter->bi_sector += bytes >> 9;

    if (bio_no_advance_iter(bio)) {
        panic("no advance iter!");
        iter->bi_size -= bytes;
    } else {
        bvec_iter_advance(bio->bi_io_vec, iter, bytes);
        /* TODO: It is reasonable to complete bio with error here. */
    }
}

#define __bio_for_each_bvec(bvl, bio, iter, start)  \
    for (iter = (start);                            \
         (iter).bi_size &&                          \
         ((bvl = mp_bvec_iter_bvec((bio)->bi_io_vec, (iter))), 1); \
         bio_advance_iter((bio), &(iter), (bvl).bv_len))

/* iterate over multi-page bvec */
#define bio_for_each_bvec(bvl, bio, iter) \
    __bio_for_each_bvec(bvl, bio, iter, (bio)->bi_iter)

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

static inline bool bio_full(struct bio *bio, unsigned len)
{
    if (bio->bi_vcnt >= bio->bi_max_vecs)
        return true;

    printk("###### %s: bi_size(%u) len(%u)\n",
           __func__, bio->bi_iter.bi_size, len);

    if (bio->bi_iter.bi_size > UINT_MAX - len)
        return true;

    printk("###### %s 2: bi_size(%u) len(%u)\n",
           __func__, bio->bi_iter.bi_size, len);

    return false;
}

int
bio_add_page(struct bio *bio, struct page *page,
             unsigned int len, unsigned int offset);

#endif /* __LINUX_BIO_H */
