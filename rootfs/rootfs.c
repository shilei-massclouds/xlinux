/* SPDX-License-Identifier: GPL-2.0-only */

#include <bug.h>
#include <vfs.h>
#include <path.h>
#include <mount.h>
#include <errno.h>
#include <ramfs.h>
#include <export.h>
#include <kernel.h>
#include <printk.h>
#include <current.h>

static int
rootfs_init_fs_context(struct fs_context *fc)
{
    return ramfs_init_fs_context(fc);
}

struct file_system_type rootfs_fs_type = {
    .name = "rootfs",
    .init_fs_context = rootfs_init_fs_context,
};

static void
init_mount_tree(void)
{
    struct path root;
    struct vfsmount *mnt;

    mnt = vfs_kern_mount(&rootfs_fs_type, 0, "rootfs", NULL);
    if (IS_ERR(mnt))
        panic("Can't create rootfs!");

    root.mnt = mnt;
    root.dentry = mnt->mnt_root;
    mnt->mnt_flags |= MNT_LOCKED;

    set_fs_pwd(current->fs, &root);
    set_fs_root(current->fs, &root);
}

static int
init_module(void)
{
    printk("module[rootfs]: init begin ...\n");
    init_mount_tree();
    printk("module[rootfs]: init end!\n");
    return 0;
}
