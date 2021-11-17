// SPDX-License-Identifier: GPL-2.0-only

#include <printk.h>

static int
init_module(void)
{
    printk("module[test_rootfs]: init begin ...\n");

    printk(_GREEN("okay!\n"));

    printk("module[test_rootfs]: init end!\n");
    return 0;
}
