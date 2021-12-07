/* SPDX-License-Identifier: GPL-2.0 */

#ifndef BLK_MQ_H
#define BLK_MQ_H

#include <blkdev.h>

struct blk_mq_tag_set {
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

#endif /* BLK_MQ_H */
