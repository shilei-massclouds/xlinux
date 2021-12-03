/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BIO_H
#define __LINUX_BIO_H

#include <gfp.h>

static inline struct bio *
bio_alloc(gfp_t gfp_mask, unsigned int nr_iovecs)
{
    panic("%s: !", __func__);
    //return bio_alloc_bioset(gfp_mask, nr_iovecs, &fs_bio_set);
}

#endif /* __LINUX_BIO_H */
