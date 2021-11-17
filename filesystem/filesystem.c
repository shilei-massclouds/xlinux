// SPDX-License-Identifier: GPL-2.0+

#include <fs.h>
#include <slab.h>
#include <errno.h>
#include <mount.h>
#include <export.h>
#include <printk.h>

void
set_fs_root(struct fs_struct *fs, const struct path *path)
{
    struct path old_root;

    old_root = fs->root;
    fs->root = *path;
    if (old_root.dentry)
        path_put(&old_root);
}
EXPORT_SYMBOL(set_fs_root);

void
set_fs_pwd(struct fs_struct *fs, const struct path *path)
{
    struct path old_pwd;

    old_pwd = fs->pwd;
    fs->pwd = *path;
    if (old_pwd.dentry)
        path_put(&old_pwd);
}
EXPORT_SYMBOL(set_fs_pwd);

static struct fs_context *
alloc_fs_context(struct file_system_type *fs_type,
                 struct dentry *reference,
                 unsigned int sb_flags,
                 unsigned int sb_flags_mask,
                 enum fs_context_purpose purpose)
{
    int (*init_fs_context)(struct fs_context *);
    struct fs_context *fc;

    fc = kzalloc(sizeof(struct fs_context), GFP_KERNEL);
    if (!fc)
        return ERR_PTR(-ENOMEM);

    fc->purpose     = purpose;
    fc->sb_flags    = sb_flags;
    fc->sb_flags_mask = sb_flags_mask;
    fc->fs_type     = get_filesystem(fs_type);

    init_fs_context = fc->fs_type->init_fs_context;
    if (!init_fs_context)
        panic("no init_fs_context!");

    if (init_fs_context(fc) < 0)
        panic("cannot init_fs_context!");

    return fc;
}

struct fs_context *
fs_context_for_mount(struct file_system_type *fs_type,
                     unsigned int sb_flags)
{
    return alloc_fs_context(fs_type, NULL,
                            sb_flags, 0, FS_CONTEXT_FOR_MOUNT);
}
EXPORT_SYMBOL(fs_context_for_mount);

void
put_fs_context(struct fs_context *fc)
{
    /* Todo: */
}
EXPORT_SYMBOL(put_fs_context);

static int
init_module(void)
{
    printk("module[filesystem]: init begin ...\n");
    printk("module[filesystem]: init end!\n");
    return 0;
}
