// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <errno.h>
#include <blk-mq.h>
#include <export.h>
#include <kernel.h>
#include <printk.h>

struct request_queue *
blk_mq_init_allocated_queue(struct blk_mq_tag_set *set,
                            struct request_queue *q,
                            bool elevator_init)
{
    return q;
}

struct request_queue *
blk_mq_init_queue_data(struct blk_mq_tag_set *set, void *queuedata)
{
    struct request_queue *uninit_q, *q;

    uninit_q = blk_alloc_queue();
    if (!uninit_q)
        return ERR_PTR(-ENOMEM);
    uninit_q->queuedata = queuedata;

    /*
     * Initialize the queue without an elevator. device_add_disk() will do
     * the initialization.
     */
    q = blk_mq_init_allocated_queue(set, uninit_q, false);
    if (IS_ERR(q))
        panic("can not init queue!");

    return q;
}
EXPORT_SYMBOL(blk_mq_init_queue_data);

struct request_queue *
blk_mq_init_queue(struct blk_mq_tag_set *set)
{
    return blk_mq_init_queue_data(set, NULL);
}
EXPORT_SYMBOL(blk_mq_init_queue);

static int
init_module(void)
{
    printk("module[blk-mq]: init begin ...\n");

    blk_dev_init();

    printk("module[blk-mq]: init end!\n");
    return 0;
}
