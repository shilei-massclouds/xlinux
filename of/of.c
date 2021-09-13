// SPDX-License-Identifier: GPL-2.0-only
#include <export.h>
#include <sbi.h>
#include <kernel.h>
#include <fdt.h>

extern void *dtb_early_va;

static int
fdt_check_header(const void *fdt)
{
    if (fdt_magic(fdt) != FDT_MAGIC)
        return -FDT_ERR_BADMAGIC;

    return 0;
}

static bool
early_init_dt_verify(void *params)
{
    sbi_printf("%s: \n", __func__);
    if (fdt_check_header(params))
        return false;

    return true;
}

static int init_module(void)
{
    sbi_puts("module[of]: init begin ...\n");
    early_init_dt_verify(dtb_early_va);
    sbi_puts("module[of]: init end!\n");
}
