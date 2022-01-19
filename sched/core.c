// SPDX-License-Identifier: GPL-2.0-only

#include <sched.h>
#include <export.h>
#include <sched/rt.h>
#include <sched/deadline.h>

/**
 * schedule_tail - first thing a freshly forked thread must call.
 * @prev: the thread we just switched away from.
 */
void _schedule_tail(struct task_struct *prev)
{
    /* Todo: add it. */
}

struct rq *
__task_rq_lock(struct task_struct *p)
{
    return task_rq(p);
}

static inline void
enqueue_task(struct rq *rq, struct task_struct *p, int flags)
{
    p->sched_class->enqueue_task(rq, p, flags);
}

void activate_task(struct rq *rq, struct task_struct *p, int flags)
{
    enqueue_task(rq, p, flags);

    p->on_rq = TASK_ON_RQ_QUEUED;
}

/*
 * wake_up_new_task - wake up a newly created task for the first time.
 *
 * This function will do some initial scheduler statistics housekeeping
 * that must be done for every newly created context, then puts the task
 * on the runqueue and wakes it.
 */
void wake_up_new_task(struct task_struct *p)
{
    struct rq *rq;

    p->state = TASK_RUNNING;

    rq = __task_rq_lock(p);

    activate_task(rq, p, ENQUEUE_NOCLOCK);

    panic("%s: !", __func__);
}
EXPORT_SYMBOL(wake_up_new_task);

int sched_fork(unsigned long clone_flags, struct task_struct *p)
{
    p->state = TASK_NEW;

    /*
     * Make sure we do not leak PI boosting priority to the child.
     */
    p->prio = current->normal_prio;

    if (dl_prio(p->prio))
        panic("bad prio %d", p->prio);
    else if (rt_prio(p->prio))
        panic("no rt sched class!");
    else
        p->sched_class = &fair_sched_class;

    p->on_cpu = 0;
    return 0;
}
EXPORT_SYMBOL(sched_fork);

static int
init_module(void)
{
    printk("module[sched]: init begin ...\n");

    schedule_tail = _schedule_tail;

    printk("module[sched]: init end!\n");

    return 0;
}
