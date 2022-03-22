// SPDX-License-Identifier: GPL-2.0-only

#include <printk.h>

static int
init_module(void)
{
    printk("module[test_serial]: init begin ...\n");
    printk("module[test_serial]: init end!\n");
}
