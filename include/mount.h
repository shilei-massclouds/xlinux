/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MOUNT_H
#define _LINUX_MOUNT_H

#include <fs.h>
#include <stat.h>
#include <dcache.h>
#include <kdev_t.h>

#define MS_RDONLY   1       /* Mount read-only */
#define MS_SILENT   32768

#define MNT_READONLY    0x40    /* does the user want this to be r/o? */
#define MNT_LOCKED      0x800000

struct vfsmount {
    struct dentry *mnt_root;    /* root of the mounted tree */
    struct super_block *mnt_sb; /* pointer to superblock */
};

struct mount {
    struct vfsmount mnt;
};

void
mnt_init(void);

int
init_mknod(const char *filename, umode_t mode, unsigned int dev);

static inline int
create_dev(char *name, dev_t dev)
{
    return init_mknod(name, S_IFBLK|0600, new_encode_dev(dev));
}

int
init_mount(const char *dev_name, const char *dir_name,
           const char *type_page, unsigned long flags);

int
kern_path(const char *name, unsigned int flags, struct path *path);

#endif /* _LINUX_MOUNT_H */
