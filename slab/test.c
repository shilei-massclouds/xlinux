// SPDX-License-Identifier: GPL-2.0-only
#include <slab.h>
#include <printk.h>
#include <string.h>

static int
kmalloc_specific_size(int size)
{
    void *p;

    p = kmalloc(size, GFP_KERNEL);
    if (p == NULL) {
        printk(_RED("kmalloc size:%d failed!\n"), size);
        return -1;
    }
    memset(p, 0, size);

    return 0;
}

static int
test_kmalloc(void)
{
/*
 * When condition is for (i = 8; i < 40960; i += 64), panic as:
 * dec32: bad instruction (0x200800cf) at (0xffffffe000003000
 */
/*
 * If limit reaches 4088, the program will halt or panic:
 * dec16: bad instruction (0x8220)
 */
    int i;

    for (i = 8; i < 4087; i += 8) {
        printk("%d\n", i);
        if (kmalloc_specific_size(i-1))
            return -1;

        if (kmalloc_specific_size(i))
            return -1;

        if (kmalloc_specific_size(i+1))
            return -1;
    }

    return 0;
}

static int
test_kfree(void)
{
    void *p;

    kfree(NULL);

    /* first malloc */
    p = kmalloc(37, GFP_KERNEL);
    if (p == NULL)
        return -1;
    kfree(p);

    /* second malloc */
    p = kmalloc(38, GFP_KERNEL);
    if (p == NULL)
        return -1;
    kfree(p);

    return 0;
}

static int
init_module(void)
{
    printk("module[test_slab]: init begin ...\n");
    printk("test slab alloc ...\n");

    if (test_kmalloc())
        printk(_RED("test slab alloc failed!\n"));
    else
        printk(_GREEN("test slab alloc ok!\n"));

    if (test_kfree())
        printk(_RED("test slab free failed!\n"));
    else
        printk(_GREEN("test slab free ok!\n"));

    printk("module[test_slab]: init end!\n");

    return 0;
}
