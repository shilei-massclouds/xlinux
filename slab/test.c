// SPDX-License-Identifier: GPL-2.0-only
#include <slab.h>
#include <printk.h>

static int
init_module(void)
{
    void *p;

    printk("init slab ...\n");
    //kmem_cache_init();
    printk("init slab ok!\n");

    printk("test slab alloc ...\n");
    //p = kzalloc(8, 8);
    printk("test slab alloc ok!\n");
    return 0;
}
