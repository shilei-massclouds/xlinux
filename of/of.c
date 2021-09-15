// SPDX-License-Identifier: GPL-2.0-only
#include <export.h>
#include <sbi.h>
#include <kernel.h>
#include <fdt.h>
#include <page.h>
#include <memblock.h>

extern void *dtb_early_va;

static bool
early_init_dt_verify(void *params)
{
    if (fdt_check_header(params))
        return false;

    /* Setup flat device-tree pointer */
    initial_boot_params = params;

    return true;
}

/**
 * early_init_dt_scan_root - fetch the top level address and size cells
 */
static int
early_init_dt_scan_root(unsigned long node,
                        const char *uname,
                        int depth,
                        void *data)
{
    const u32 *prop;

    if (depth != 0)
        return 0;

    dt_root_size_cells = OF_ROOT_NODE_SIZE_CELLS_DEFAULT;
    dt_root_addr_cells = OF_ROOT_NODE_ADDR_CELLS_DEFAULT;

    prop = of_get_flat_dt_prop(node, "#size-cells", NULL);
    if (prop)
        dt_root_size_cells = be32_to_cpup(prop);

    prop = of_get_flat_dt_prop(node, "#address-cells", NULL);
    if (prop)
        dt_root_addr_cells = be32_to_cpup(prop);

    /* break now */
    return 1;
}

#define MIN_MEMBLOCK_ADDR   __pa(PAGE_OFFSET)
#define MAX_MEMBLOCK_ADDR   ((phys_addr_t)~0)

static void
early_init_dt_add_memory_arch(u64 base, u64 size)
{
    const u64 phys_offset = MIN_MEMBLOCK_ADDR;

    if (size < PAGE_SIZE - (base & ~PAGE_MASK)) {
        sbi_printf("Ignoring memory block %lx - %lx\n",
                   base, base + size);
        return;
    }

    if (!PAGE_ALIGNED(base)) {
        size -= PAGE_SIZE - (base & ~PAGE_MASK);
        base = PAGE_ALIGN(base);
    }
    size &= PAGE_MASK;

    if (base > MAX_MEMBLOCK_ADDR) {
        sbi_printf("Ignoring memory block %lx - %lx\n",
                   base, base + size);
        return;
    }

    if (base + size - 1 > MAX_MEMBLOCK_ADDR) {
        sbi_printf("Ignoring memory range %lx - %lx\n",
                   ((u64)MAX_MEMBLOCK_ADDR) + 1, base + size);
        size = MAX_MEMBLOCK_ADDR - base + 1;
    }

    if (base + size < phys_offset) {
        sbi_printf("Ignoring memory block %lx - %lx\n",
                   base, base + size);
        return;
    }

    if (base < phys_offset) {
        sbi_printf("Ignoring memory range %lx - %lx\n",
                   base, phys_offset);
        size -= phys_offset - base;
        base = phys_offset;
    }

    memblock_add(base, size);
}

static u64
dt_mem_next_cell(int s, const u32 **cellp)
{
    const u32 *p = *cellp;

    *cellp = p + s;
    return of_read_number(p, s);
}

/**
 * early_init_dt_scan_memory - Look for and parse memory nodes
 */
static int
early_init_dt_scan_memory(unsigned long node,
                          const char *uname,
                          int depth,
                          void *data)
{
    const u32 *reg;
    const u32 *endp;
    const char *type;
    int len;

    /* We are scanning "memory" nodes only */
    type = of_get_flat_dt_prop(node, "device_type", NULL);
    if (type == NULL || strcmp(type, "memory") != 0)
        return 0;

    reg = of_get_flat_dt_prop(node, "reg", &len);
    if (reg == NULL)
        return 0;

    endp = reg + (len / sizeof(u32));

    sbi_printf("memory scan node %s, reg size %d,\n", uname, len);

    while ((endp - reg) >= (dt_root_addr_cells + dt_root_size_cells)) {
        u64 base, size;

        base = dt_mem_next_cell(dt_root_addr_cells, &reg);
        size = dt_mem_next_cell(dt_root_size_cells, &reg);

        if (size == 0)
            continue;

        sbi_printf(" - %lx ,  %lx\n",
                   (unsigned long long)base,
                   (unsigned long long)size);

        early_init_dt_add_memory_arch(base, size);
    }

    return 0;
}

static void
early_init_dt_scan_nodes(void)
{
    of_scan_flat_dt(early_init_dt_scan_root, NULL);

    of_scan_flat_dt(early_init_dt_scan_memory, NULL);
}

static int init_module(void)
{
    sbi_puts("module[of]: init begin ...\n");
    early_init_dt_verify(dtb_early_va);
    sbi_puts("module[of]: scan dtb nodes ...\n");
    early_init_dt_scan_nodes();
    sbi_puts("module[of]: init end!\n");
}
