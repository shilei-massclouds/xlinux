// SPDX-License-Identifier: GPL-2.0-only

#include <fs.h>
#include <bug.h>
#include <stat.h>
#include <printk.h>

static int
test_create_dir(void)
{
    umode_t mode;
    mode = S_IFDIR;
    mode |= S_IRWXU;
    mode |= (S_IRGRP | S_IXGRP);
    mode |= (S_IROTH | S_IXOTH);
    return init_mkdir("dev", mode);
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
