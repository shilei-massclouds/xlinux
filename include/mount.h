/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _LINUX_MOUNT_H
#define _LINUX_MOUNT_H

#include <fs.h>

#define MNT_LOCKED      0x800000

struct dentry {
};

struct vfsmount {
    struct dentry *mnt_root;    /* root of the mounted tree */
    struct super_block *mnt_sb; /* pointer to superblock */
    int mnt_flags;
};

#endif /* _LINUX_MOUNT_H */
