// SPDX-License-Identifier: GPL-2.0-only
#include <printk.h>
#include <string.h>
#include <vmalloc.h>

static int
test_vmalloc(void)
{
    void *p;

    p = vmalloc(8000);
    if (!p)
        return -1;

    memset(p, 0, 8000);
    return 0;
}

static int
init_module(void)
{
    printk("module[test_vmalloc]: init begin ...\n");

    if(test_vmalloc())
        printk(_RED("vmalloc failed!\n"));
    else
        printk(_GREEN("vmalloc okay!\n"));

    printk("module[test_vmalloc]: init end!\n");
    return 0;
}
