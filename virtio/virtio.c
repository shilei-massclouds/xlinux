// SPDX-License-Identifier: GPL-2.0+

#include <printk.h>
#include <virtio.h>

static int
init_module(void)
{
    printk("module[virtio]: init begin ...\n");
    printk("module[virtio]: init end!\n");
    return 0;
}
