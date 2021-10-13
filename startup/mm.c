// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
#include <page.h>
#include <export.h>
#include <linkage.h>
#include <pgtable.h>

#ifndef __riscv_cmodel_medany
#error "Don't use absolute addressing now."
#endif

pge_t early_pgd[PTRS_PER_PGD] __initdata __aligned(PAGE_SIZE);
EXPORT_SYMBOL(early_pgd);
pme_t early_pmd[PTRS_PER_PMD] __initdata __aligned(PAGE_SIZE);
EXPORT_SYMBOL(early_pmd);
pme_t fixmap_pmd[PTRS_PER_PMD] __page_aligned_bss;
EXPORT_SYMBOL(fixmap_pmd);
pte_t fixmap_pt[PTRS_PER_PT] __page_aligned_bss;
EXPORT_SYMBOL(fixmap_pt);
pge_t swapper_pgd[PTRS_PER_PGD] __page_aligned_bss;
EXPORT_SYMBOL(swapper_pgd);

unsigned long pfn_base;
EXPORT_SYMBOL(pfn_base);

unsigned long va_pa_offset;
EXPORT_SYMBOL(va_pa_offset);

phys_addr_t dtb_early_pa __initdata;
EXPORT_SYMBOL(dtb_early_pa);

extern char _start[];

void setup_early_pge(uintptr_t dtb_pa)
{
    uintptr_t load_pa = (uintptr_t)(&_start);
    uintptr_t pge_idx = pge_index(PAGE_OFFSET);
    uintptr_t pme_idx = pme_index(PAGE_OFFSET);

    va_pa_offset = PAGE_OFFSET - load_pa;
    pfn_base = PFN_DOWN(load_pa);

    early_pgd[pge_idx] =
        pfn_pge(PFN_DOWN((uintptr_t)early_pmd), PAGE_TABLE);

    early_pmd[pme_idx] =
        pfn_pme(PFN_DOWN(load_pa), PAGE_KERNEL_EXEC);

    /* Setup flash address space for loading modules. */
    pge_idx = pge_index(FLASH_VA);
    early_pgd[pge_idx] = pfn_pge(PFN_DOWN(FLASH_PA), PAGE_KERNEL);

    dtb_early_pa = dtb_pa;
}
