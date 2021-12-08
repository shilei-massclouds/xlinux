/* SPDX-License-Identifier: GPL-2.0 */

#ifndef BLK_MQ_H
#define BLK_MQ_H

#include <blkdev.h>

#define queue_for_each_hw_ctx(q, hctx, i)               \
    for ((i) = 0; (i) < (q)->nr_hw_queues &&            \
         ({ hctx = (q)->queue_hw_ctx[i]; 1; }); (i)++)

enum {
    BLK_MQ_F_SHOULD_MERGE   = 1 << 0,
    BLK_MQ_F_TAG_SHARED = 1 << 1,
    /*
     * Set when this device requires underlying blk-mq device for
     * completing IO:
     */
    BLK_MQ_F_STACKING   = 1 << 2,
    BLK_MQ_F_BLOCKING   = 1 << 5,
    BLK_MQ_F_NO_SCHED   = 1 << 6,
    BLK_MQ_F_ALLOC_POLICY_START_BIT = 8,
    BLK_MQ_F_ALLOC_POLICY_BITS = 1,

    BLK_MQ_S_STOPPED    = 0,
    BLK_MQ_S_TAG_ACTIVE = 1,
    BLK_MQ_S_SCHED_RESTART  = 2,

    /* hw queue is inactive after all its CPUs become offline */
    BLK_MQ_S_INACTIVE   = 3,

    BLK_MQ_MAX_DEPTH    = 10240,

    BLK_MQ_CPU_WORK_BATCH   = 8,
};
#define BLK_MQ_FLAG_TO_ALLOC_POLICY(flags) \
    ((flags >> BLK_MQ_F_ALLOC_POLICY_START_BIT) & \
     ((1 << BLK_MQ_F_ALLOC_POLICY_BITS) - 1))

enum {
    BLK_MQ_NO_TAG       = -1U,
    BLK_MQ_TAG_MIN      = 1,
    BLK_MQ_TAG_MAX      = BLK_MQ_NO_TAG - 1,
};

struct blk_mq_tags {
    unsigned int nr_tags;
    unsigned int nr_reserved_tags;

    struct request **rqs;
    struct request **static_rqs;
    struct list_head page_list;
};

struct blk_mq_hw_ctx {
    struct blk_mq_tags *sched_tags;
};

struct blk_mq_ops {
    /**
     * @init_request: Called for every command allocated by the block layer
     * to allow the driver to set up driver specific data.
     *
     * Tag greater than or equal to queue_depth is for setting up
     * flush request.
     */
    int (*init_request)(struct blk_mq_tag_set *set,
                        struct request *,
                        unsigned int);
};

struct blk_mq_tag_set {
    const struct blk_mq_ops *ops;
    unsigned int nr_hw_queues;
    unsigned int queue_depth;
    unsigned int reserved_tags;
    unsigned int cmd_size;
    unsigned int flags;
    void *driver_data;
};

struct blk_mq_alloc_data {
    struct request_queue *q;
    unsigned int cmd_flags;
};

struct request_queue *
blk_mq_init_queue(struct blk_mq_tag_set *set);

struct request_queue *blk_alloc_queue();

int blk_dev_init(void);

blk_qc_t blk_mq_submit_bio(struct bio *bio);

/**
 * blk_mq_rq_to_pdu - cast a request to a PDU
 * @rq: the request to be casted
 *
 * Return: pointer to the PDU
 *
 * Driver command data is immediately after the request. So add request to get
 * the PDU.
 */
static inline void *blk_mq_rq_to_pdu(struct request *rq)
{
    return rq + 1;
}

#endif /* BLK_MQ_H */
