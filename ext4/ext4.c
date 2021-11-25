// SPDX-License-Identifier: GPL-2.0+

#include <fs.h>
#include <bug.h>
#include <export.h>
#include <printk.h>

bool ext4_initialized = false;
EXPORT_SYMBOL(ext4_initialized);

static struct dentry *
ext4_mount(struct file_system_type *fs_type, int flags, const char *dev_name)
{
    panic("%s: ", __func__);
    //return mount_bdev(fs_type, flags, dev_name, data, ext4_fill_super);
}

static struct file_system_type ext4_fs_type = {
    .name       = "ext4",
    .mount      = ext4_mount,
    .fs_flags   = FS_REQUIRES_DEV,
};

static int
init_module(void)
{
    printk("module[ext4]: init begin ...\n");

    register_filesystem(&ext4_fs_type);
    ext4_initialized = true;

    printk("module[ext4]: init end!\n");
    return 0;
}
