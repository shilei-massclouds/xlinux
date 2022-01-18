// SPDX-License-Identifier: GPL-2.0-only

#include <sched.h>
#include <export.h>

/**
 * schedule_tail - first thing a freshly forked thread must call.
 * @prev: the thread we just switched away from.
 */
void _schedule_tail(struct task_struct *prev)
{
    /* Todo: add it. */
}

static int
init_module(void)
{
    printk("module[sched]: init begin ...\n");

    schedule_tail = _schedule_tail;

    printk("module[sched]: init end!\n");

    return 0;
}
