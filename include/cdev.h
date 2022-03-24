/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_CDEV_H
#define _LINUX_CDEV_H

#include <fs.h>
#include <device.h>

struct cdev {
    struct kobject kobj;
    const struct file_operations *ops;
    struct list_head list;
    dev_t dev;
    unsigned int count;
};

int cdev_add(struct cdev *p, dev_t dev, unsigned count);

struct cdev *cdev_alloc(void);

#endif /* _LINUX_CDEV_H */
