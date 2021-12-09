// SPDX-License-Identifier: GPL-2.0

#include <bug.h>
#include <slab.h>
#include <blk-mq.h>
#include <blkdev.h>
#include <export.h>
#include <workqueue.h>

struct kmem_cache *blk_requestq_cachep;

/*
 * Controlling structure to kblockd
 */
static struct workqueue_struct *kblockd_workqueue;

int
kblockd_mod_delayed_work_on(int cpu,
                            struct delayed_work *dwork,
                            unsigned long delay)
{
    return mod_delayed_work_on(cpu, kblockd_workqueue, dwork, delay);
}
EXPORT_SYMBOL(kblockd_mod_delayed_work_on);

static blk_qc_t __submit_bio_noacct_mq(struct bio *bio)
{
    return blk_mq_submit_bio(bio);
}

blk_qc_t submit_bio_noacct(struct bio *bio)
{
    BUG_ON(bio->bi_disk->fops->submit_bio);
    return __submit_bio_noacct_mq(bio);
}
EXPORT_SYMBOL(submit_bio_noacct);

blk_qc_t submit_bio(struct bio *bio)
{
    return submit_bio_noacct(bio);
}
EXPORT_SYMBOL(submit_bio);

struct request_queue *
blk_alloc_queue(void)
{
    int ret;
    struct request_queue *q;

    q = kmem_cache_alloc(blk_requestq_cachep, GFP_KERNEL|__GFP_ZERO);
    if (!q)
        return NULL;

    return q;
}
EXPORT_SYMBOL(blk_alloc_queue);

int blk_dev_init(void)
{
    /* used for unplugging and affects IO latency/throughput - HIGHPRI */
    kblockd_workqueue = alloc_workqueue("kblockd",
                                        WQ_MEM_RECLAIM|WQ_HIGHPRI, 0);
    if (!kblockd_workqueue)
        panic("Failed to create kblockd\n");

    blk_requestq_cachep =
        kmem_cache_create("request_queue",
                          sizeof(struct request_queue),
                          0, SLAB_PANIC, NULL);

    return 0;
}
