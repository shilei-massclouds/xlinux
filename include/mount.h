/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MOUNT_H
#define _LINUX_MOUNT_H

#include <fs.h>
#include <dcache.h>

#define MNT_LOCKED      0x800000

struct vfsmount {
    struct dentry *mnt_root;    /* root of the mounted tree */
    struct super_block *mnt_sb; /* pointer to superblock */
    int mnt_flags;
};

struct mount {
    struct vfsmount mnt;
};

void
mnt_init(void);

#endif /* _LINUX_MOUNT_H */
