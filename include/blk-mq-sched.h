/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _BLK_MQ_INIT_SCHED_H
#define _BLK_MQ_INIT_SCHED_H

#include <elevator.h>

int blk_mq_init_sched(struct request_queue *q, struct elevator_type *e);

#endif /* _BLK_MQ_INIT_SCHED_H */
