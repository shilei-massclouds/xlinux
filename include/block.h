/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <blkdev.h>

void
blk_queue_logical_block_size(struct request_queue *q, unsigned int size);

#endif /* _BLOCK_H_ */
