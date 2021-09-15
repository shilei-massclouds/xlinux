// SPDX-License-Identifier: GPL-2.0-only
#include <sbi.h>
#include <export.h>
#include <memblock.h>

int
memblock_add(phys_addr_t base, phys_addr_t size)
{
    sbi_printf("%s: [%lx-%lx]\n", __func__, base, size);
    return 0;
}
EXPORT_SYMBOL(memblock_add);

static int init_module(void)
{
    sbi_puts("module[memblock]: init begin ...\n");
    sbi_puts("module[memblock]: init end!\n");
}
