// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <slab.h>
#include <errno.h>
#include <blk-mq.h>
#include <export.h>
#include <kernel.h>
#include <printk.h>
#include <jiffies.h>
#include <elevator.h>
#include <blk-mq-sched.h>

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

    hctx->queue = q;
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

static int blk_mq_alloc_ctxs(struct request_queue *q)
{
    struct blk_mq_ctxs *ctxs;

    ctxs = kzalloc(sizeof(*ctxs), GFP_KERNEL);
    if (!ctxs)
        return -ENOMEM;

    ctxs->queue_ctx = kzalloc(sizeof(struct blk_mq_ctx), GFP_KERNEL);
    if (!ctxs->queue_ctx)
        panic("out of memory!");

    ctxs->queue_ctx->ctxs = ctxs;

    //q->mq_kobj = &ctxs->kobj;
    q->queue_ctx = ctxs->queue_ctx;

    return 0;
}

static void blk_mq_map_swqueue(struct request_queue *q)
{
    int i;
    struct blk_mq_ctx *ctx = q->queue_ctx;
    for (i = 0; i < HCTX_TYPE_POLL; i++)
        ctx->hctxs[i] = q->queue_hw_ctx[0];
}

struct request_queue *
blk_mq_init_allocated_queue(struct blk_mq_tag_set *set,
                            struct request_queue *q,
                            bool elevator_init)
{
    if (blk_mq_alloc_ctxs(q))
        panic("bad ctxs!");

    blk_mq_realloc_hw_ctxs(set, q);
    if (!q->nr_hw_queues)
        panic("nr_hw_queues ZERO!");

    q->tag_set = set;
    blk_mq_map_swqueue(q);
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
blk_mq_rq_ctx_init(struct blk_mq_alloc_data *data,
                   unsigned int tag,
                   u64 alloc_time_ns)
{
    struct blk_mq_tags *tags = blk_mq_tags_from_data(data);
    struct request *rq = tags->static_rqs[tag];

    rq->q = data->q;
    rq->mq_ctx = data->ctx;
    rq->mq_hctx = data->hctx;
    rq->rq_flags = 0;
    rq->cmd_flags = data->cmd_flags;
    INIT_LIST_HEAD(&rq->queuelist);
    return rq;
}

static struct request *
__blk_mq_alloc_request(struct blk_mq_alloc_data *data)
{
    unsigned int tag;
    struct request_queue *q = data->q;

    data->ctx = blk_mq_get_ctx(q);
    data->hctx = blk_mq_map_queue(q, data->cmd_flags, data->ctx);

    /* Todo: */
    tag = 1;
    return blk_mq_rq_ctx_init(data, tag, 0);
}

static bool blk_mq_hctx_has_pending(struct blk_mq_hw_ctx *hctx)
{
    return blk_mq_sched_has_work(hctx);
}

static void
__blk_mq_delay_run_hw_queue(struct blk_mq_hw_ctx *hctx,
                            bool async,
                            unsigned long msecs)
{
    kblockd_mod_delayed_work_on(0, &hctx->run_work, msecs_to_jiffies(msecs));
    panic("%s: async(%d)!", __func__, async);
}

void blk_mq_run_hw_queue(struct blk_mq_hw_ctx *hctx, bool async)
{
    bool need_run;

    need_run = blk_mq_hctx_has_pending(hctx);

    if (need_run)
        __blk_mq_delay_run_hw_queue(hctx, async, 0);

    panic("%s: need_run(%d)!", __func__, need_run);
}
EXPORT_SYMBOL(blk_mq_run_hw_queue);

void
blk_mq_sched_insert_request(struct request *rq,
                            bool at_head,
                            bool run_queue,
                            bool async)
{
    struct request_queue *q = rq->q;
    struct elevator_queue *e = q->elevator;
    struct blk_mq_hw_ctx *hctx = rq->mq_hctx;
    LIST_HEAD(list);

    list_add(&rq->queuelist, &list);
    e->type->ops.insert_requests(hctx, &list, at_head);

    if (run_queue)
        blk_mq_run_hw_queue(hctx, async);
}

blk_qc_t blk_mq_submit_bio(struct bio *bio)
{
    struct request *rq;
    struct request_queue *q = bio->bi_disk->queue;
    struct blk_mq_alloc_data data = {
        .q = q,
    };

    data.cmd_flags = bio->bi_opf;
    rq = __blk_mq_alloc_request(&data);
    if (unlikely(!rq))
        panic("request is NULL!");

    /*
    cookie = request_to_qc_t(data.hctx, rq);

    blk_mq_bio_to_request(rq, bio, nr_segs);
    */

    /* Insert the request at the IO scheduler queue */
    blk_mq_sched_insert_request(rq, false, true, true);

    panic("%s: !", __func__);
    return 0;
}
EXPORT_SYMBOL(blk_mq_submit_bio);

static int
init_module(void)
{
    printk("module[blk-mq]: init begin ...\n");
    printk("module[blk-mq]: init end!\n");
    return 0;
}
