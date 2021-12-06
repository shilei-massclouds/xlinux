/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BVEC_ITER_H
#define __LINUX_BVEC_ITER_H

#include <page.h>

struct bio_vec {
    struct page *bv_page;
    unsigned int    bv_len;
    unsigned int    bv_offset;
};

#endif /* __LINUX_BVEC_ITER_H */
