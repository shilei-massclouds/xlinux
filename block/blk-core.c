// SPDX-License-Identifier: GPL-2.0

#include <bug.h>
#include <bio.h>
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

static void req_bio_endio(struct request *rq, struct bio *bio,
                          unsigned int nbytes, blk_status_t error)
{
    if (error)
        bio->bi_status = error;

    bio_advance(bio, nbytes);
}

bool blk_update_request(struct request *req, blk_status_t error,
                        unsigned int nr_bytes)
{
    int total_bytes;

    if (!req->bio)
        return false;

    printk("%s: 0\n", __func__);

    while (req->bio) {
        struct bio *bio = req->bio;
        unsigned bio_bytes = min(bio->bi_iter.bi_size, nr_bytes);

        printk("%s: 1 bio(%lx) (%u, %u)\n",
               __func__, req->bio, bio_bytes, bio->bi_iter.bi_size);

        if (bio_bytes == bio->bi_iter.bi_size)
            req->bio = bio->bi_next;

        /* Completion has already been traced */
        bio_clear_flag(bio, BIO_TRACE_COMPLETION);
        req_bio_endio(req, bio, bio_bytes, error);

        total_bytes += bio_bytes;
        nr_bytes -= bio_bytes;

        printk("%s: 1 bio(%lx) (%u)\n",
               __func__, req->bio, nr_bytes);

        if (!nr_bytes)
            break;
    }

    /*
     * completely done
     */
    if (!req->bio) {
        /*
         * Reset counters so that the request stacking driver
         * can find how many bytes remain in the request
         * later.
         */
        req->__data_len = 0;
        return false;
    }

    panic("%s: nr_bytes(%u)!", __func__, nr_bytes);
}

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
