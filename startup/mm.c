// SPDX-License-Identifier: GPL-2.0-only

#include <types.h>
#include <page.h>
#include <linkage.h>
#include <fixmap.h>
#include <pgtable.h>
#include <bug.h>
#include <export.h>

#ifndef __riscv_cmodel_medany
#error "Don't use absolute addressing now."
#endif

extern char _start[];

pge_t early_pgd[PTRS_PER_PGD] __initdata __aligned(PAGE_SIZE);
pme_t early_pmd[PTRS_PER_PMD] __initdata __aligned(PAGE_SIZE);
pme_t fixmap_pmd[PTRS_PER_PMD] __page_aligned_bss;
pte_t fixmap_pt[PTRS_PER_PT] __page_aligned_bss;

void *dtb_early_va;
EXPORT_SYMBOL(dtb_early_va);

static phys_addr_t dtb_early_pa __initdata;

void setup_early_pge(void)
{
    uintptr_t load_pa = (uintptr_t)(&_start);
    uintptr_t pge_idx = pge_index(PAGE_OFFSET);
    uintptr_t pme_idx = pme_index(PAGE_OFFSET);

    BUG_ON(!pge_none(early_pgd[pge_idx]));

    early_pgd[pge_idx] =
        pfn_pge(PFN_DOWN((uintptr_t)early_pmd), PAGE_TABLE);

    early_pmd[pme_idx] =
        pfn_pme(PFN_DOWN(load_pa), PAGE_KERNEL_EXEC);
}

void setup_fixmap_pge(uintptr_t dtb_pa)
{
    uintptr_t va, end_va;
    uintptr_t load_pa = (uintptr_t)(&_start);
    uintptr_t pge_idx = pge_index(FIXADDR_START);
    uintptr_t pme_idx = pme_index(FIXADDR_START);

    BUG_ON(!pge_none(early_pgd[pge_idx]));
    BUG_ON(!pme_none(fixmap_pmd[pme_idx]));

    early_pgd[pge_idx] =
        pfn_pge(PFN_DOWN((uintptr_t)fixmap_pmd), PAGE_TABLE);

    fixmap_pmd[pme_idx] =
        pfn_pme(PFN_DOWN((uintptr_t)fixmap_pt), PAGE_TABLE);

    end_va = fix_to_virt(FIX_FDT) + FIX_FDT_SIZE;
    for (va = fix_to_virt(FIX_FDT); va < end_va; va += PAGE_SIZE) {
        uintptr_t pte_idx = pte_index(va);
        uintptr_t pa = dtb_pa + (va - fix_to_virt(FIX_FDT));
        fixmap_pt[pte_idx] = pfn_pte(PFN_DOWN(pa), PAGE_KERNEL);
    }

    dtb_early_va = (void *)fix_to_virt(FIX_FDT) + (dtb_pa & ~PAGE_MASK);
    dtb_early_pa = dtb_pa;
}

void setup_flash_pge(void)
{
    uintptr_t pge_idx = pge_index(FLASH_VA);
    BUG_ON(!pge_none(early_pgd[pge_idx]));

    early_pgd[pge_idx] = pfn_pge(PFN_DOWN(FLASH_PA), PAGE_KERNEL);
}
