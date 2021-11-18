// SPDX-License-Identifier: GPL-2.0+

#include <slab.h>
#include <stat.h>
#include <dcache.h>
#include <errno.h>
#include <ramfs.h>
#include <types.h>
#include <export.h>
#include <string.h>

#define RAMFS_DEFAULT_MODE  0755

struct ramfs_mount_opts {
    umode_t mode;
};

struct ramfs_fs_info {
    struct ramfs_mount_opts mount_opts;
};

static struct file_system_type *file_systems;

struct inode *
ramfs_get_inode(struct super_block *sb, const struct inode *dir,
                umode_t mode, dev_t dev)
{
    struct inode *inode = new_inode(sb);
    return inode;
}

static const struct super_operations ramfs_ops = {
};

static int
ramfs_fill_super(struct super_block *sb, struct fs_context *fc)
{
    struct inode *inode;
    struct ramfs_fs_info *fsi = sb->s_fs_info;

    sb->s_op = &ramfs_ops;

    inode = ramfs_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
    sb->s_root = d_make_root(inode);
    if (!sb->s_root)
        return -ENOMEM;

    return 0;
}

static int
ramfs_get_tree(struct fs_context *fc)
{
    return get_tree_nodev(fc, ramfs_fill_super);
}

static const struct fs_context_operations ramfs_context_ops = {
    .get_tree = ramfs_get_tree,
};

int
ramfs_init_fs_context(struct fs_context *fc)
{
    struct ramfs_fs_info *fsi;

    fsi = kzalloc(sizeof(*fsi), GFP_KERNEL);
    if (!fsi)
        return -ENOMEM;

    fsi->mount_opts.mode = RAMFS_DEFAULT_MODE;
    fc->s_fs_info = fsi;
    fc->ops = &ramfs_context_ops;
    return 0;
}
EXPORT_SYMBOL(ramfs_init_fs_context);

static struct file_system_type ramfs_fs_type = {
    .name = "ramfs",
    .init_fs_context = ramfs_init_fs_context,
    .fs_flags = FS_USERNS_MOUNT,
};

static struct file_system_type **
find_filesystem(const char *name, unsigned len)
{
    struct file_system_type **p;
    for (p = &file_systems; *p; p = &(*p)->next)
        if (strncmp((*p)->name, name, len) == 0 && !(*p)->name[len])
            break;
    return p;
}

int
register_filesystem(struct file_system_type * fs)
{
    struct file_system_type ** p;

    BUG_ON(strchr(fs->name, '.'));
    if (fs->next)
        return -EBUSY;

    p = find_filesystem(fs->name, strlen(fs->name));
    if (*p)
        return -EBUSY;

    *p = fs;
    return 0;
}
EXPORT_SYMBOL(register_filesystem);

static int
init_module(void)
{
    printk("module[ramfs]: init begin ...\n");

    register_filesystem(&ramfs_fs_type);

    printk("module[ramfs]: init end!\n");
    return 0;
}
