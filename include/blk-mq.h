/* SPDX-License-Identifier: GPL-2.0 */

#ifndef BLK_MQ_H
#define BLK_MQ_H

#include <blkdev.h>

struct blk_mq_tag_set {
};

struct request_queue *
blk_mq_init_queue(struct blk_mq_tag_set *set);

struct request_queue *blk_alloc_queue();

int blk_dev_init(void);

#endif /* BLK_MQ_H */
