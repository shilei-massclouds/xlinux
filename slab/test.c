// SPDX-License-Identifier: GPL-2.0-only
#include <slab.h>
#include <printk.h>

static int
init_module(void)
{
    void *p;

    printk("test slab ...\n");
    p = kzalloc(8, 8);
    return 0;
}
