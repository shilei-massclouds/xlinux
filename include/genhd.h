/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_GENHD_H
#define _LINUX_GENHD_H

#include <sysfs.h>
#include <device.h>

#define DISK_MAX_PARTS          256
#define DISK_NAME_LEN           32

#define GENHD_FL_EXT_DEVT           0x0040

struct gendisk {
    /* major, first_minor and minors are input parameters only,
     * don't use directly.  Use disk_devt() and disk_max_parts().
     */
    int major;          /* major number of driver */
    int first_minor;
    int minors;         /* maximum number of minors, =1 for
                         * disks that can't be partitioned. */
    char disk_name[DISK_NAME_LEN];  /* name of major driver */

    const struct block_device_operations *fops;
    struct request_queue *queue;
    void *private_data;

    int flags;
};

extern struct gendisk *__alloc_disk_node(int minors);

#define alloc_disk_node(minors)                  \
({                                               \
    const char *__name;                          \
    struct gendisk *__disk;                      \
                                                 \
    __name = "(gendisk_completion)"#minors"(0)"; \
                                        \
    __disk = __alloc_disk_node(minors); \
    __disk;                             \
})

#define alloc_disk(minors) alloc_disk_node(minors)

int
register_blkdev(unsigned int major, const char *name);

static inline sector_t get_capacity(struct gendisk *disk)
{
    return 0;
    /* return disk->part0.nr_sects; */
    /* Todo */
}

static inline void set_capacity(struct gendisk *disk, sector_t size)
{
    /* disk->part0.nr_sects = size; */
    /* Todo */
}

void
device_add_disk(struct device *parent,
                struct gendisk *disk,
                const struct attribute_group **groups);

#endif /* _LINUX_GENHD_H */
