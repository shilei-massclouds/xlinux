// SPDX-License-Identifier: GPL-2.0-only

#include <slab.h>
#include <errno.h>
#include <export.h>
#include <printk.h>
#include <workqueue.h>

struct workqueue_struct {
};

struct workqueue_struct *
alloc_workqueue(const char *fmt, unsigned int flags, int max_active)
{
    struct workqueue_struct *wq;
    size_t tbl_size = 0;

    wq = kzalloc(sizeof(*wq) + tbl_size, GFP_KERNEL);
    if (!wq)
        return NULL;

    return wq;
}
EXPORT_SYMBOL(alloc_workqueue);

static int
try_to_grab_pending(struct work_struct *work, bool is_dwork,
                    unsigned long *flags)
{
    BUG_ON(!is_dwork);

    /* try to claim PENDING the normal way */
    if (!test_and_set_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(work)))
        return 0;

    panic("no pending!");
}

bool
mod_delayed_work_on(int cpu,
                    struct workqueue_struct *wq,
                    struct delayed_work *dwork,
                    unsigned long delay)
{
    int ret;
    unsigned long flags;

    do {
        ret = try_to_grab_pending(&dwork->work, true, &flags);
    } while (unlikely(ret == -EAGAIN));

    /*
    if (likely(ret >= 0)) {
        __queue_delayed_work(cpu, wq, dwork, delay);
        local_irq_restore(flags);
    }
    */
    panic("%s: !", __func__);

    /* -ENOENT from try_to_grab_pending() becomes %true */
    return ret;
}
EXPORT_SYMBOL(mod_delayed_work_on);

static int
init_module(void)
{
    printk("module[workqueue]: init begin ...\n");
    printk("module[workqueue]: init end!\n");
    return 0;
}
