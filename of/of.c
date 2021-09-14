// SPDX-License-Identifier: GPL-2.0-only
#include <export.h>
#include <sbi.h>
#include <kernel.h>
#include <fdt.h>

extern void *dtb_early_va;

static int init_module(void)
{
    sbi_puts("module[of]: init begin ...\n");
    early_init_dt_verify(dtb_early_va);
    sbi_puts("module[of]: init end!\n");
}
