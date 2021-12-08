// SPDX-License-Identifier: GPL-2.0

#include <slab.h>
#include <errno.h>
#include <elevator.h>

struct deadline_data {
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

    /*
    INIT_LIST_HEAD(&dd->fifo_list[READ]);
    INIT_LIST_HEAD(&dd->fifo_list[WRITE]);
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

static struct elevator_type mq_deadline = {
    .ops = {
        .init_sched = dd_init_queue,
    },
    .elevator_name = "mq-deadline",
};

int deadline_init(void)
{
    return elv_register(&mq_deadline);
}
