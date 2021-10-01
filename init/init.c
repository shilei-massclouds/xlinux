// SPDX-License-Identifier: GPL-2.0-only
#include <mm.h>
#include <bug.h>
#include <fdt.h>
#include <printk.h>
#include <of_platform.h>

extern u32 kernel_size;

static void
start_kernel(void)
{
    printk("start_kernel: init ...\n");

    if (kernel_size >= PME_SIZE)
        panic("kernel size (%lu) is too large!\n", kernel_size);

    printk("start_kernel: init ok!\n");
}

static int
init_module(void)
{
    printk("module[init]: init begin ...\n");

    start_kernel_fn = start_kernel;

    printk("module[init]: init end!\n");

    return 0;
}