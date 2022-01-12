/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __LINUX_UIO_H
#define __LINUX_UIO_H

#include <types.h>

struct iovec
{
    void *iov_base;
    __kernel_size_t iov_len;
};

enum iter_type {
    /* iter types */
    ITER_IOVEC = 4,
    ITER_KVEC = 8,
    ITER_BVEC = 16,
    ITER_PIPE = 32,
    ITER_DISCARD = 64,
};

struct iov_iter {
    /*
     * Bit 0 is the read/write bit, set if we're writing.
     * Bit 1 is the BVEC_FLAG_NO_REF bit, set if type is a bvec and
     * the caller isn't expecting to drop a page reference when done.
     */
    unsigned int type;
    size_t iov_offset;
    size_t count;
    union {
        const struct iovec *iov;
        const struct kvec *kvec;
    };
    unsigned long nr_segs;
};

static inline size_t iov_iter_count(const struct iov_iter *i)
{
    return i->count;
}

void iov_iter_init(struct iov_iter *i, unsigned int direction,
                   const struct iovec *iov, unsigned long nr_segs,
                   size_t count);

/*
 * Cap the iov_iter by given limit; note that the second argument is
 * *not* the new size - it's upper limit for such.  Passing it a value
 * greater than the amount of data in iov_iter is fine - it'll just do
 * nothing in that case.
 */
static inline void iov_iter_truncate(struct iov_iter *i, u64 count)
{
    /*
     * count doesn't have to fit in size_t - comparison extends both
     * operands to u64 here and any value that would be truncated by
     * conversion in assignement is by definition greater than all
     * values of size_t, including old i->count.
     */
    if (i->count > count)
        i->count = count;
}

#endif /* __LINUX_UIO_H */
