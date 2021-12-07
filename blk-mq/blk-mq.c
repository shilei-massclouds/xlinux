// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <errno.h>
#include <blk-mq.h>
#include <export.h>
#include <kernel.h>
#include <printk.h>

static void
blk_mq_realloc_hw_ctxs(struct blk_mq_tag_set *set,
                       struct request_queue *q)
{
    q->nr_hw_queues = set->nr_hw_queues;
}

struct request_queue *
blk_mq_init_allocated_queue(struct blk_mq_tag_set *set,
                            struct request_queue *q,
                            bool elevator_init)
{
    blk_mq_realloc_hw_ctxs(set, q);
    if (!q->nr_hw_queues)
        panic("nr_hw_queues ZERO!");

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

static struct request *
__blk_mq_alloc_request(struct blk_mq_alloc_data *data)
{
    panic("%s: !", __func__);
}

blk_qc_t blk_mq_submit_bio(struct bio *bio)
{
    blk_qc_t cookie;
    struct request *rq;
    struct request_queue *q = bio->bi_disk->queue;
    struct blk_mq_alloc_data data = {
        .q = q,
    };

    data.cmd_flags = bio->bi_opf;
    rq = __blk_mq_alloc_request(&data);
    if (unlikely(!rq))
        panic("request is NULL!");

    panic("%s: !", __func__);
    /*
    cookie = request_to_qc_t(data.hctx, rq);

    blk_mq_bio_to_request(rq, bio, nr_segs);
    */

    /* Insert the request at the IO scheduler queue */
    /*
    blk_mq_sched_insert_request(rq, false, true, true);

    return cookie;
    */
}
EXPORT_SYMBOL(blk_mq_submit_bio);

static int
init_module(void)
{
    printk("module[blk-mq]: init begin ...\n");

    blk_dev_init();

    printk("module[blk-mq]: init end!\n");
    return 0;
}
