// SPDX-License-Identifier: GPL-2.0

#include <export.h>
#include <blk-mq-sched.h>

int blk_mq_init_sched(struct request_queue *q, struct elevator_type *e)
{
    panic("%s: !", __func__);
}
EXPORT_SYMBOL(blk_mq_init_sched);
