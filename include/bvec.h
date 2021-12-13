/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BVEC_ITER_H
#define __LINUX_BVEC_ITER_H

#include <bug.h>
#include <page.h>

#define __bvec_iter_bvec(bvec, iter)    (&(bvec)[(iter).bi_idx])

/* multi-page (mp_bvec) helpers */
#define mp_bvec_iter_page(bvec, iter)   \
    (__bvec_iter_bvec((bvec), (iter))->bv_page)

#define mp_bvec_iter_len(bvec, iter)    \
    min((iter).bi_size,                 \
        __bvec_iter_bvec((bvec), (iter))->bv_len - (iter).bi_bvec_done)

#define mp_bvec_iter_offset(bvec, iter) \
    (__bvec_iter_bvec((bvec), (iter))->bv_offset + (iter).bi_bvec_done)

#define mp_bvec_iter_bvec(bvec, iter)   \
((struct bio_vec) {                     \
    .bv_page    = mp_bvec_iter_page((bvec), (iter)),    \
    .bv_len     = mp_bvec_iter_len((bvec), (iter)),     \
    .bv_offset  = mp_bvec_iter_offset((bvec), (iter)),  \
})

struct bio_vec {
    struct page     *bv_page;
    unsigned int    bv_len;
    unsigned int    bv_offset;
};

struct bvec_iter {
    sector_t        bi_sector;  /* device address in 512 byte sectors */
    unsigned int    bi_size;    /* residual I/O count */
    unsigned int    bi_idx;     /* current index into bvl_vec */

    /* number of bytes completed in current bvec */
    unsigned int    bi_bvec_done;
};

static inline bool
bvec_iter_advance(const struct bio_vec *bv,
                  struct bvec_iter *iter, unsigned bytes)
{
    unsigned int idx = iter->bi_idx;

    BUG_ON(bytes > iter->bi_size);

    iter->bi_size -= bytes;
    bytes += iter->bi_bvec_done;

    while (bytes && bytes >= bv[idx].bv_len) {
        bytes -= bv[idx].bv_len;
        idx++;
    }

    iter->bi_idx = idx;
    iter->bi_bvec_done = bytes;
    return true;
}

#endif /* __LINUX_BVEC_ITER_H */
