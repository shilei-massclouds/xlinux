// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <uio.h>
#include <export.h>
#include <kernel.h>
#include <printk.h>
#include <uaccess.h>

void iov_iter_init(struct iov_iter *i, unsigned int direction,
                   const struct iovec *iov, unsigned long nr_segs,
                   size_t count)
{
    BUG_ON(direction & ~(READ | WRITE));
    direction &= READ | WRITE;

    /* It will get better.  Eventually... */
    if (uaccess_kernel()) {
        i->type = ITER_KVEC | direction;
        i->kvec = (struct kvec *)iov;
    } else {
        i->type = ITER_IOVEC | direction;
        i->iov = iov;
    }
    i->nr_segs = nr_segs;
    i->iov_offset = 0;
    i->count = count;
}
EXPORT_SYMBOL(iov_iter_init);

static int
init_module(void)
{
    printk("module[iov_iter]: init begin ...\n");
    printk("module[iov_iter]: init end!\n");
    return 0;
}
