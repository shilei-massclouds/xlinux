// SPDX-License-Identifier: GPL-2.0-only

#include <types.h>
#include <page.h>
#include <linkage.h>

#ifndef __riscv_cmodel_medany
#error "Don't use absolute addressing now."
#endif

extern char _start[];

pge_t trampoline_pgd[PTRS_PER_PGD] __page_aligned_bss;
pme_t trampoline_pmd[PTRS_PER_PMD] __page_aligned_bss;

void setup_trampoline(void)
{
    uintptr_t load_pa = (uintptr_t)(&_start);
    uintptr_t pge_idx = pge_index(PAGE_OFFSET);
    uintptr_t pme_idx = pme_index(PAGE_OFFSET);

    trampoline_pgd[pge_idx] =
        pfn_pge(PFN_DOWN((uintptr_t)trampoline_pmd), PAGE_TABLE);

    trampoline_pmd[pme_idx] =
        pfn_pme(PFN_DOWN(load_pa), PAGE_KERNEL_EXEC);
}
