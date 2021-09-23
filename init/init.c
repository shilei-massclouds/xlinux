// SPDX-License-Identifier: GPL-2.0-only
#include <fdt.h>
#include <sbi.h>
#include <memblock.h>

static void
start_kernel(void)
{
    sbi_puts("start_kernel: init ...\n");

    early_init_dt_verify();
    early_init_dt_scan_nodes();

    memblock_setup_vm_final();

    unflatten_device_tree();

    of_platform_default_populate_init();

    sbi_puts("start_kernel: init ok!\n");
}

static int
init_module(void)
{
    sbi_puts("module[init]: init begin ...\n");

    start_kernel_fn = start_kernel;

    sbi_puts("module[init]: init end!\n");

    return 0;
}
