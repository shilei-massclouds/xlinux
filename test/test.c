// SPDX-License-Identifier: GPL-2.0-only
#include <export.h>
#include <sbi.h>
#include <kernel.h>

static void test(void)
{
    sbi_puts("start_kernel test!\n");
}
EXPORT_SYMBOL(test);

static int init_module(void)
{
    sbi_puts("test_print: [\n");
    start_kernel = test;
    sbi_puts("]\n");
}
