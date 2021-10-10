// SPDX-License-Identifier: GPL-2.0-only
#include <slab.h>
#include <printk.h>

static int
init_module(void)
{
    void *p;

    printk("test slab alloc ...\n");

    p = kzalloc(8, 8);
    if (p == NULL)
        printk(_RED("test slab alloc failed!\n"));
    else
        printk(_GREEN("test slab alloc ok!\n"));

    return 0;
}
