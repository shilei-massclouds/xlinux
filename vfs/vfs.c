/* SPDX-License-Identifier: GPL-2.0-only */

#include <fs.h>
#include <errno.h>
#include <export.h>
#include <kernel.h>
#include <printk.h>

int
vfs_get_tree(struct fs_context *fc)
{
    return 0;
}
EXPORT_SYMBOL(vfs_get_tree);

struct vfsmount *
vfs_create_mount(struct fs_context *fc)
{
}

struct vfsmount *
fc_mount(struct fs_context *fc)
{
    int err = vfs_get_tree(fc);
    if (err) {
        return ERR_PTR(err);
    }
    return vfs_create_mount(fc);
}
EXPORT_SYMBOL(fc_mount);

struct vfsmount *
vfs_kern_mount(struct file_system_type *type,
               int flags, const char *name, void *data)
{
    struct fs_context *fc;
    struct vfsmount *mnt;
    int ret = 0;

    if (!type)
        return ERR_PTR(-EINVAL);

    fc = fs_context_for_mount(type, flags);
    if (IS_ERR(fc))
        return ERR_CAST(fc);

    /*
    if (name)
        ret = vfs_parse_fs_string(fc, "source", name, strlen(name));
    if (!ret)
        ret = parse_monolithic_mount_data(fc, data);
    */
    if (!ret)
        mnt = fc_mount(fc);
    else
        mnt = ERR_PTR(ret);

    put_fs_context(fc);
    return mnt;
}
EXPORT_SYMBOL(vfs_kern_mount);

static int
init_module(void)
{
    printk("module[vfs]: init begin ...\n");
    printk("module[vfs]: init end!\n");
    return 0;
}
