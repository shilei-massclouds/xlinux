/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_ELEVATOR_H
#define _LINUX_ELEVATOR_H

#include <blkdev.h>

struct elevator_type {
    const char *elevator_name;
    struct list_head list;
};

int elv_register(struct elevator_type *e);

void elevator_init_mq(struct request_queue *q);

#endif /* _LINUX_ELEVATOR_H */
