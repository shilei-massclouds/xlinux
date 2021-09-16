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

void *
memblock_alloc_try_nid(phys_addr_t size, phys_addr_t align)
{
    void *ptr;

    sbi_printf("%s: %lu bytes align=%lx\n",
               __func__, (u64)size, (u64)align);

    //ptr = memblock_alloc_internal(size, align);
    ptr = NULL;
    if (ptr)
        memset(ptr, 0, size);

    return ptr;
}
EXPORT_SYMBOL(memblock_alloc_try_nid);

static int init_module(void)
{
    sbi_puts("module[memblock]: init begin ...\n");
    sbi_puts("module[memblock]: init end!\n");
}
