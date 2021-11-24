// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <slab.h>
#include <class.h>
#include <genhd.h>
#include <blkdev.h>
#include <export.h>
#include <kdev_t.h>
#include <string.h>

#define BLKDEV_MAJOR_HASH_SIZE 255
static struct blk_major_name {
    struct blk_major_name *next;
    int major;
    char name[16];
} *major_names[BLKDEV_MAJOR_HASH_SIZE];

struct class block_class = {
    .name = "block",
};

const struct device_type disk_type = {
    .name = "disk",
};

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
        disk->minors = minors;
        disk_to_dev(disk)->class = &block_class;
        disk_to_dev(disk)->type = &disk_type;
        device_initialize(disk_to_dev(disk));
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
    struct device *ddev = disk_to_dev(disk);

    ddev->parent = parent;

    dev_set_name(ddev, "%s", disk->disk_name);

    if (device_add(ddev))
        return;

    /* Todo: */
    panic("%s: reach here!", __func__);
}

int
blk_alloc_devt(struct hd_struct *part, dev_t *devt)
{
    struct gendisk *disk = part_to_disk(part);

    if (part->partno < disk->minors) {
        *devt = MKDEV(disk->major, disk->first_minor + part->partno);
        return 0;
    }

    panic("bad partno (%d)", part->partno);
}

static void
__device_add_disk(struct device *parent,
                  struct gendisk *disk,
                  const struct attribute_group **groups,
                  bool register_queue)
{
    dev_t devt;
    int retval;

    retval = blk_alloc_devt(&disk->part0, &devt);
    BUG_ON(retval);

    disk->major = MAJOR(devt);
    disk->first_minor = MINOR(devt);

    printk("%s: (%x) ma(%x) mi(%x)\n",
           __func__, devt, disk->major, disk->first_minor);

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
    BUG_ON(!slab_is_available());
    BUG_ON(class_register(&block_class));
    printk("module[genhd]: init end!\n");
    return 0;
}
