// SPDX-License-Identifier: GPL-2.0+

#include <fs.h>
#include <bug.h>
#include <export.h>
#include <printk.h>

bool ext2_initialized = false;
EXPORT_SYMBOL(ext2_initialized);

static int
ext2_fill_super(struct super_block *sb, void *data, int silent)
{
    panic("%s: ", __func__);
}

static struct dentry *
ext2_mount(struct file_system_type *fs_type, int flags, const char *dev_name)
{
    return mount_bdev(fs_type, flags, dev_name, ext2_fill_super);
}

static struct file_system_type ext2_fs_type = {
    .name       = "ext2",
    .mount      = ext2_mount,
    .fs_flags   = FS_REQUIRES_DEV,
};

static int
init_module(void)
{
    printk("module[ext2]: init begin ...\n");

    register_filesystem(&ext2_fs_type);
    ext2_initialized = true;

    printk("module[ext2]: init end!\n");
    return 0;
}
