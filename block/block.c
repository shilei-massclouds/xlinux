// SPDX-License-Identifier: GPL-2.0+

#include <block.h>
#include <export.h>
#include <printk.h>

void
blk_queue_flag_set(unsigned int flag, struct request_queue *q)
{
    set_bit(flag, &q->queue_flags);
}
EXPORT_SYMBOL(blk_queue_flag_set);

void
blk_queue_flag_clear(unsigned int flag, struct request_queue *q)
{
    clear_bit(flag, &q->queue_flags);
}
EXPORT_SYMBOL(blk_queue_flag_clear);

void
wbt_set_write_cache(struct request_queue *q, bool write_cache_on)
{
    /* Todo: */
}

void
blk_queue_max_segments(struct request_queue *q,
                       unsigned short max_segments)
{
    if (!max_segments) {
        max_segments = 1;
        printk("%s: set to minimum %d\n", __func__, max_segments);
    }

    q->limits.max_segments = max_segments;
}
EXPORT_SYMBOL(blk_queue_max_segments);

blk_qc_t submit_bio(struct bio *bio)
{
    panic("%s: !", __func__);
}
EXPORT_SYMBOL(submit_bio);

static int
init_module(void)
{
    printk("module[block]: init begin ...\n");
    printk("module[block]: init end!\n");
    return 0;
}
