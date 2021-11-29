/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _LINUX_BLKDEV_H
#define _LINUX_BLKDEV_H

#include <types.h>

#define BLKDEV_MAJOR_MAX    512

struct block_device {
    dev_t bd_dev;  /* not a kdev_t - it's a search key */
    struct super_block *bd_super;
    struct inode *bd_inode;     /* will die */
    struct gendisk *bd_disk;
};

struct block_device_operations {
    /*
    blk_qc_t (*submit_bio) (struct bio *bio);
    */
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
    unsigned short      max_segments;
};

struct request_queue {
    struct queue_limits limits;

    /*
     * various queue flags, see QUEUE_* below
     */
    unsigned long       queue_flags;
};

void
blk_queue_flag_set(unsigned int flag, struct request_queue *q);

void
blk_queue_flag_clear(unsigned int flag, struct request_queue *q);

void
blk_queue_max_segments(struct request_queue *q,
                       unsigned short max_segments);

#endif /* _LINUX_BLKDEV_H */
