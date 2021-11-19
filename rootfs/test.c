// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <namei.h>
#include <printk.h>

static int
test_create_dir(void)
{
    struct path path;
    struct dentry *dentry;

    dentry = kern_path_create(AT_FDCWD, "dev", &path, LOOKUP_DIRECTORY);
    if (!dentry)
        return -1;
    return 0;
}

static int
init_module(void)
{
    printk("module[test_rootfs]: init begin ...\n");

    BUG_ON(!rootfs_initialized);

    if (test_create_dir()) {
        printk(_RED("test dir create failed!\n"));
        return -1;
    }

    printk("module[test_rootfs]: init end!\n");
    return 0;
}
