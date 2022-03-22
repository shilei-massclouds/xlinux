// SPDX-License-Identifier: GPL-2.0

#include <fs.h>
#include <device.h>
#include <export.h>
#include <kobj_map.h>

static struct kobj_map *cdev_map;

static struct kobject *exact_match(dev_t dev, int *part, void *data)
{
    struct cdev *p = data;
    return &p->kobj;
}

/**
 * cdev_add() - add a char device to the system
 * @p: the cdev structure for the device
 * @dev: the first device number for which this device is responsible
 * @count: the number of consecutive minor numbers corresponding to this
 *         device
 *
 * cdev_add() adds the device represented by @p to the system, making it
 * live immediately.  A negative error code is returned on failure.
 */
int cdev_add(struct cdev *p, dev_t dev, unsigned count)
{
    int error;

    p->dev = dev;
    p->count = count;

    if (dev == WHITEOUT_DEV)
        return -EBUSY;

    error = kobj_map(cdev_map, dev, count, exact_match, p);
    if (error)
        return error;

    return 0;
}

static int chrdev_open(struct inode *inode, struct file *filp)
{
    struct cdev *p;

    p = inode->i_cdev;
    if (!p) {
        int idx;
        struct kobject *kobj;
        kobj = kobj_lookup(cdev_map, inode->i_rdev, &idx);
        if (!kobj) {
            panic("no i_rdev(%lx)", inode->i_rdev);
            return -ENXIO;
        }
    }

    panic("no char device open!");
}

/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */
const struct file_operations def_chr_fops = {
    .open = chrdev_open,
};
EXPORT_SYMBOL(def_chr_fops);

static struct kobject *base_probe(dev_t dev, int *part, void *data)
{
    return NULL;
}

void chrdev_init(void)
{
    cdev_map = kobj_map_init(base_probe);
}
