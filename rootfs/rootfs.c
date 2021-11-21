/* SPDX-License-Identifier: GPL-2.0-only */

#include <bug.h>
#include <vfs.h>
#include <path.h>
#include <errno.h>
#include <mount.h>
#include <namei.h>
#include <ramfs.h>
#include <export.h>
#include <kernel.h>
#include <printk.h>
#include <current.h>

bool rootfs_initialized = false;
EXPORT_SYMBOL(rootfs_initialized);

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

int
init_mkdir(const char *pathname, umode_t mode)
{
    struct path path;
    struct dentry *dentry;

    dentry = kern_path_create(AT_FDCWD, pathname, &path,
                              LOOKUP_DIRECTORY);
    if (IS_ERR(dentry))
        return PTR_ERR(dentry);

    return vfs_mkdir(path.dentry->d_inode, dentry, mode);
}
EXPORT_SYMBOL(init_mkdir);

static int
init_module(void)
{
    printk("module[rootfs]: init begin ...\n");
    init_mount_tree();
    rootfs_initialized = true;
    printk("module[rootfs]: init end!\n");
    return 0;
}
