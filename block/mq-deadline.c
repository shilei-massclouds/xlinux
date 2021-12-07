// SPDX-License-Identifier: GPL-2.0

#include <elevator.h>

static struct elevator_type mq_deadline = {
    .elevator_name = "mq-deadline",
};

int deadline_init(void)
{
    return elv_register(&mq_deadline);
}
