// SPDX-License-Identifier: GPL-2.0

#include <list.h>
#include <slab.h>
#include <errno.h>
#include <blkdev.h>
#include <blk-mq.h>
#include <elevator.h>

struct deadline_data {
    struct list_head fifo_list[2];
};

static int
dd_init_queue(struct request_queue *q,
              struct elevator_type *e)
{
    struct deadline_data *dd;
    struct elevator_queue *eq;

    eq = elevator_alloc(q, e);
    if (!eq)
        return -ENOMEM;

    dd = kzalloc(sizeof(*dd), GFP_KERNEL);
    if (!dd) {
        kobject_put(&eq->kobj);
        return -ENOMEM;
    }
    eq->elevator_data = dd;

    INIT_LIST_HEAD(&dd->fifo_list[READ]);
    INIT_LIST_HEAD(&dd->fifo_list[WRITE]);
    /*
    dd->sort_list[READ] = RB_ROOT;
    dd->sort_list[WRITE] = RB_ROOT;
    dd->fifo_expire[READ] = read_expire;
    dd->fifo_expire[WRITE] = write_expire;
    dd->writes_starved = writes_starved;
    dd->front_merges = 1;
    dd->fifo_batch = fifo_batch;
    INIT_LIST_HEAD(&dd->dispatch);
    */

    q->elevator = eq;
    return 0;
}

static void
dd_insert_request(struct blk_mq_hw_ctx *hctx,
                  struct request *rq,
                  bool at_head)
{
    struct request_queue *q = hctx->queue;
    struct deadline_data *dd = q->elevator->elevator_data;
    const int data_dir = rq_data_dir(rq);

    BUG_ON(at_head);

    list_add_tail(&rq->queuelist, &dd->fifo_list[data_dir]);
}

static void
dd_insert_requests(struct blk_mq_hw_ctx *hctx,
                   struct list_head *list, bool at_head)
{
    struct request_queue *q = hctx->queue;
    struct deadline_data *dd = q->elevator->elevator_data;

    while (!list_empty(list)) {
        struct request *rq;

        rq = list_first_entry(list, struct request, queuelist);
        list_del_init(&rq->queuelist);
        dd_insert_request(hctx, rq, at_head);
    }
}

static bool dd_has_work(struct blk_mq_hw_ctx *hctx)
{
    struct deadline_data *dd = hctx->queue->elevator->elevator_data;

    return !list_empty_careful(&dd->fifo_list[0]) ||
        !list_empty_careful(&dd->fifo_list[1]);
}

static struct elevator_type mq_deadline = {
    .ops = {
        .insert_requests    = dd_insert_requests,
        .has_work           = dd_has_work,
        .init_sched         = dd_init_queue,
    },
    .elevator_name = "mq-deadline",
};

int deadline_init(void)
{
    return elv_register(&mq_deadline);
}
