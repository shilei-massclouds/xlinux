/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _LINUX_BLKDEV_H
#define _LINUX_BLKDEV_H

#include <fs.h>
#include <types.h>
#include <genhd.h>
#include <blk_types.h>

#define BLKDEV_MAJOR_MAX    512

typedef unsigned int blk_qc_t;

enum req_opf {
    /* read sectors from the device */
    REQ_OP_READ     = 0,

    REQ_OP_LAST,
};

struct block_device {
    dev_t bd_dev;  /* not a kdev_t - it's a search key */
    struct super_block *bd_super;
    struct inode *bd_inode;     /* will die */
    struct gendisk *bd_disk;
    struct hd_struct *bd_part;
    u8 bd_partno;
    void *bd_claiming;
};

struct block_device_operations {
    blk_qc_t (*submit_bio) (struct bio *bio);
    int (*open)(struct block_device *, fmode_t);
    /*
    void (*release) (struct gendisk *, fmode_t);
    int (*rw_page)(struct block_device *, sector_t, struct page *, unsigned int);
    int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
    int (*compat_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
    unsigned int (*check_events) (struct gendisk *disk,
                      unsigned int clearing);
    void (*unlock_native_capacity) (struct gendisk *);
    int (*revalidate_disk) (struct gendisk *);
    int (*getgeo)(struct block_device *, struct hd_geometry *);
    void (*swap_slot_free_notify) (struct block_device *, unsigned long);
    int (*report_zones)(struct gendisk *, sector_t sector,
            unsigned int nr_zones, report_zones_cb cb, void *data);
    char *(*devnode)(struct gendisk *disk, umode_t *mode);
    struct module *owner;
    const struct pr_ops *pr_ops;
    */
};

struct queue_limits {
    unsigned int        physical_block_size;
    unsigned int        logical_block_size;
    unsigned short      max_segments;
};

struct request_queue {
    struct queue_limits limits;

    /*
     * various queue flags, see QUEUE_* below
     */
    unsigned long queue_flags;

    void *queuedata;
};

void
blk_queue_flag_set(unsigned int flag, struct request_queue *q);

void
blk_queue_flag_clear(unsigned int flag, struct request_queue *q);

void
blk_queue_max_segments(struct request_queue *q,
                       unsigned short max_segments);

static inline unsigned
queue_logical_block_size(const struct request_queue *q)
{
    int retval = 512;

    if (q && q->limits.logical_block_size)
        retval = q->limits.logical_block_size;

    return retval;
}

static inline struct request_queue *
bdev_get_queue(struct block_device *bdev)
{
    return bdev->bd_disk->queue;    /* this is never NULL */
}

static inline unsigned int
bdev_logical_block_size(struct block_device *bdev)
{
    return queue_logical_block_size(bdev_get_queue(bdev));
}

/* assumes size > 256 */
static inline unsigned int
blksize_bits(unsigned int size)
{
    unsigned int bits = 8;
    do {
        bits++;
        size >>= 1;
    } while (size > 256);
    return bits;
}

static inline unsigned int
block_size(struct block_device *bdev)
{
    return 1 << bdev->bd_inode->i_blkbits;
}

struct block_device *I_BDEV(struct inode *inode);

#endif /* _LINUX_BLKDEV_H */
