// SPDX-License-Identifier: GPL-2.0

#include <sched.h>

/* Walk up scheduling entities hierarchy */
#define for_each_sched_entity(se) for(; se; se = se->parent)

/* runqueue on which this entity is (to be) queued */
static inline struct cfs_rq *cfs_rq_of(struct sched_entity *se)
{
    return se->cfs_rq;
}

static inline int
entity_before(struct sched_entity *a, struct sched_entity *b)
{
    return (s64)(a->vruntime - b->vruntime) < 0;
}

/*
 * Enqueue an entity into the rb-tree:
 */
static void __enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    struct sched_entity *entry;
    bool leftmost = true;
    struct rb_node *parent = NULL;
    struct rb_node **link = &cfs_rq->tasks_timeline.rb_root.rb_node;

    /*
     * Find the right place in the rbtree:
     */
    while (*link) {
        parent = *link;
        entry = rb_entry(parent, struct sched_entity, run_node);
        /*
         * We dont care about collisions. Nodes with
         * the same key stay together.
         */
        if (entity_before(se, entry)) {
            link = &parent->rb_left;
        } else {
            link = &parent->rb_right;
            leftmost = false;
        }
    }

    rb_link_node(&se->run_node, parent, link);
    rb_insert_color_cached(&se->run_node, &cfs_rq->tasks_timeline, leftmost);
}

static void
enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{
    bool curr = cfs_rq->curr == se;

    if (!curr)
        __enqueue_entity(cfs_rq, se);
    se->on_rq = 1;
}

void init_cfs_rq(struct cfs_rq *cfs_rq)
{
    cfs_rq->tasks_timeline = RB_ROOT_CACHED;
}

void init_tg_cfs_entry(struct task_group *tg, struct cfs_rq *cfs_rq,
                       struct sched_entity *se, int cpu,
                       struct sched_entity *parent)
{
    struct rq *rq = cpu_rq();

    cfs_rq->tg = tg;
    cfs_rq->rq = rq;

    tg->cfs_rq[cpu] = cfs_rq;
    tg->se[cpu] = se;

    /* se could be NULL for root_task_group */
    if (!se)
        return;

    if (!parent) {
        se->cfs_rq = &rq->cfs;
    } else {
        se->cfs_rq = parent->my_q;
    }

    se->my_q = cfs_rq;
    se->parent = parent;
}

/*
 * The enqueue_task method is called before nr_running is
 * increased. Here we update the fair scheduling stats and
 * then put the task into the rbtree:
 */
static void
enqueue_task_fair(struct rq *rq, struct task_struct *p, int flags)
{
    struct cfs_rq *cfs_rq;
    struct sched_entity *se = &p->se;

    for_each_sched_entity(se) {
        if (se->on_rq)
            break;

        cfs_rq = cfs_rq_of(se);
        printk("%s: cfs_rq(%lx)\n", __func__, cfs_rq);
        enqueue_entity(cfs_rq, se, flags);

        flags = ENQUEUE_WAKEUP;
    }
}

/*
 * All the scheduling class methods:
 */
const struct sched_class fair_sched_class = {
    .enqueue_task = enqueue_task_fair,
};
