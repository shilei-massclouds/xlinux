// SPDX-License-Identifier: GPL-2.0

#include <sched.h>

/*
 * The enqueue_task method is called before nr_running is
 * increased. Here we update the fair scheduling stats and
 * then put the task into the rbtree:
 */
static void
enqueue_task_fair(struct rq *rq, struct task_struct *p, int flags)
{
    panic("%s: !", __func__);
}

/*
 * All the scheduling class methods:
 */
const struct sched_class fair_sched_class = {
    .enqueue_task = enqueue_task_fair,
};
