// SPDX-License-Identifier: GPL-2.0-only
#include <export.h>
#include <sbi.h>
#include <kernel.h>
#include <page.h>
#include <memblock.h>
#include <pgtable.h>

static void test(void)
{
    sbi_puts("start_kernel test!\n");
}
EXPORT_SYMBOL(test);

static int init_module(void)
{
    sbi_puts("module[test]: init begin ...\n");

    start_kernel_fn = test;

    sbi_puts("module[test]: init end!\n");

    return 0;
}