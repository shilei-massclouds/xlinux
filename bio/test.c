// SPDX-License-Identifier: GPL-2.0-only
#include <printk.h>

static int
init_module(void)
{
    printk("module[test_bio]: init begin ...\n");
    printk("module[test_bio]: init end!\n");
    return 0;
}
