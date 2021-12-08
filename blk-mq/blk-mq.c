// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <slab.h>
#include <errno.h>
#include <blk-mq.h>
#include <export.h>
#include <kernel.h>
#include <printk.h>

static int blk_mq_hw_ctx_size(struct blk_mq_tag_set *tag_set)
{
    int hw_ctx_size = sizeof(struct blk_mq_hw_ctx);
    return hw_ctx_size;
}

static struct blk_mq_hw_ctx *
blk_mq_alloc_hctx(struct request_queue *q, struct blk_mq_tag_set *set)
{
    struct blk_mq_hw_ctx *hctx;
    gfp_t gfp = GFP_NOIO | __GFP_NOWARN | __GFP_NORETRY;

    hctx = kzalloc(blk_mq_hw_ctx_size(set), gfp);
    if (!hctx)
        panic("out of memory!");

    return hctx;
}

static int
blk_mq_init_hctx(struct request_queue *q,
                 struct blk_mq_tag_set *set,
                 struct blk_mq_hw_ctx *hctx, unsigned hctx_idx)
{
    /* Todo */
}

static struct blk_mq_hw_ctx *
blk_mq_alloc_and_init_hctx(struct blk_mq_tag_set *set,
                           struct request_queue *q,
                           int hctx_idx)
{
    struct blk_mq_hw_ctx *hctx;

    hctx = blk_mq_alloc_hctx(q, set);
    if (!hctx)
        panic("out of memory!");

    if (blk_mq_init_hctx(q, set, hctx, hctx_idx))
        panic("can not init hctx!");

    return hctx;
}

static void
blk_mq_realloc_hw_ctxs(struct blk_mq_tag_set *set,
                       struct request_queue *q)
{
    int i;
    struct blk_mq_hw_ctx **hctxs;

    BUG_ON(q->nr_hw_queues);

    hctxs = kcalloc(set->nr_hw_queues, sizeof(*hctxs), GFP_KERNEL);
    if (!hctxs)
        panic("out of memory!");

    q->queue_hw_ctx = hctxs;
    q->nr_hw_queues = set->nr_hw_queues;

    for (i = 0; i < set->nr_hw_queues; i++) {
        struct blk_mq_hw_ctx *hctx;

        if (hctxs[i])
            continue;

        hctx = blk_mq_alloc_and_init_hctx(set, q, i);
        if (hctx)
            hctxs[i] = hctx;
        else
            panic("Allocate new hctx fails");
    }
}

struct request_queue *
blk_mq_init_allocated_queue(struct blk_mq_tag_set *set,
                            struct request_queue *q,
                            bool elevator_init)
{
    blk_mq_realloc_hw_ctxs(set, q);
    if (!q->nr_hw_queues)
        panic("nr_hw_queues ZERO!");

    q->tag_set = set;
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
