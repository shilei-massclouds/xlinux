// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <slab.h>
#include <genhd.h>
#include <blkdev.h>
#include <export.h>
#include <string.h>

#define BLKDEV_MAJOR_HASH_SIZE 255
static struct blk_major_name {
    struct blk_major_name *next;
    int major;
    char name[16];
} *major_names[BLKDEV_MAJOR_HASH_SIZE];

struct gendisk *
__alloc_disk_node(int minors)
{
    struct gendisk *disk;

    if (minors > DISK_MAX_PARTS) {
        panic("block: can't allocate more than %d partitions\n",
              DISK_MAX_PARTS);
    }

    disk = kzalloc_node(sizeof(struct gendisk), GFP_KERNEL);
    if (disk) {
        /* Todo: */
    }

    return disk;
}
EXPORT_SYMBOL(__alloc_disk_node);

static inline int
major_to_index(unsigned major)
{
    return major % BLKDEV_MAJOR_HASH_SIZE;
}

static void
register_disk(struct device *parent,
              struct gendisk *disk,
              const struct attribute_group **groups)
{
    /* Todo: */
    panic("%s: reach here!", __func__);
}

static void
__device_add_disk(struct device *parent,
                  struct gendisk *disk,
                  const struct attribute_group **groups,
                  bool register_queue)
{
    register_disk(parent, disk, groups);
}

void
device_add_disk(struct device *parent,
                struct gendisk *disk,
                const struct attribute_group **groups)
{
    __device_add_disk(parent, disk, groups, true);
}
EXPORT_SYMBOL(device_add_disk);

int
register_blkdev(unsigned int major, const char *name)
{
    int index;
    struct blk_major_name *p;
    struct blk_major_name **n;
    int ret = 0;

    /* temporary */
    if (major == 0) {
        for (index = ARRAY_SIZE(major_names)-1; index > 0; index--) {
            if (major_names[index] == NULL)
                break;
        }

        if (index == 0)
            panic("%s: failed to get major for %s\n", __func__, name);

        major = index;
        ret = major;
    }

    if (major >= BLKDEV_MAJOR_MAX)
        panic("%s: major requested (%u) is greater than the max(%u) for %s",
              __func__, major, BLKDEV_MAJOR_MAX-1, name);

    p = kmalloc(sizeof(struct blk_major_name), GFP_KERNEL);
    if (p == NULL)
        panic("NO memory!");

    p->major = major;
    strlcpy(p->name, name, sizeof(p->name));
    p->next = NULL;
    index = major_to_index(major);

    for (n = &major_names[index]; *n; n = &(*n)->next) {
        if ((*n)->major == major)
            break;
    }
    if (!*n)
        *n = p;
    else
        panic("BUSY!");

    if (ret < 0)
        panic("register_blkdev: cannot get major %u for %s\n",
              major, name);

    return ret;
}
EXPORT_SYMBOL(register_blkdev);

static int
init_module(void)
{
    printk("module[genhd]: init begin ...\n");
    printk("module[genhd]: init end!\n");
    return 0;
}
