/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SYSFS_H_
#define _SYSFS_H_

struct attribute {
    const char  *name;
    umode_t     mode;
};

struct sysfs_ops {
    ssize_t (*show)(struct kobject *, struct attribute *, char *);
    ssize_t (*store)(struct kobject *, struct attribute *, const char *, size_t);
};

#endif /* _SYSFS_H_ */
