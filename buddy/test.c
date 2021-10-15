// SPDX-License-Identifier: GPL-2.0-only
#include <gfp.h>
#include <printk.h>

static int
init_module(void)
{
    struct page *page;

    printk("module[test_buddy]: init begin ...\n");
    page = alloc_page(GFP_KERNEL);
    if(!page)
        panic("bad alloc!\n");

    printk(_GREEN("alloc one page okay!\n"));

    printk("module[test_buddy]: init end!\n");
    return 0;
}
