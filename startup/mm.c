// SPDX-License-Identifier: GPL-2.0-only

#include <types.h>
#include <page.h>
#include <linkage.h>
#include <fixmap.h>
#include <pgtable.h>
#include <bug.h>
#include <export.h>
#include <kernel.h>
#include <memblock.h>
#include <mm.h>
#include <csr.h>

#ifndef __riscv_cmodel_medany
#error "Don't use absolute addressing now."
#endif

static phys_alloc_t phys_alloc_fn;

extern char _start[];

pge_t swapper_pgd[PTRS_PER_PGD] __page_aligned_bss;

pge_t early_pgd[PTRS_PER_PGD] __initdata __aligned(PAGE_SIZE);
pme_t early_pmd[PTRS_PER_PMD] __initdata __aligned(PAGE_SIZE);
pme_t fixmap_pmd[PTRS_PER_PMD] __page_aligned_bss;
pte_t fixmap_pt[PTRS_PER_PT] __page_aligned_bss;

unsigned long va_pa_offset;
EXPORT_SYMBOL(va_pa_offset);

void *dtb_early_va;
EXPORT_SYMBOL(dtb_early_va);

static phys_addr_t dtb_early_pa __initdata;
static bool mmu_enabled;

void setup_early_pge(void)
{
    uintptr_t load_pa = (uintptr_t)(&_start);
    uintptr_t pge_idx = pge_index(PAGE_OFFSET);
    uintptr_t pme_idx = pme_index(PAGE_OFFSET);

    va_pa_offset = PAGE_OFFSET - load_pa;

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

void
setup_flash_pge(void)
{
    uintptr_t pge_idx = pge_index(FLASH_VA);

    if (mmu_enabled) {
        BUG_ON(!pge_none(swapper_pgd[pge_idx]));
        swapper_pgd[pge_idx] = pfn_pge(PFN_DOWN(FLASH_PA), PAGE_KERNEL);
    } else {
        BUG_ON(!pge_none(early_pgd[pge_idx]));
        early_pgd[pge_idx] = pfn_pge(PFN_DOWN(FLASH_PA), PAGE_KERNEL);
    }
}

void
clear_flash_pge(void)
{
    uintptr_t pge_idx = pge_index(FLASH_VA);
    BUG_ON(!mmu_enabled);
    swapper_pgd[pge_idx] = __pge(0);
}

void
__set_fixmap(enum fixed_addresses idx, phys_addr_t phys, pgprot_t prot)
{
    unsigned long addr = fix_to_virt(idx);
    pte_t *ptep;

    BUG_ON(idx <= FIX_HOLE || idx >= __end_of_fixed_addresses);

    ptep = &fixmap_pt[pte_index(addr)];

    if (pgprot_val(prot)) {
        set_pte(ptep, pfn_pte(phys >> PAGE_SHIFT, prot));
    } else {
        set_pte(ptep, __pte(0));
        local_flush_tlb_page(addr);
    }
}

static pme_t *
get_pmd_virt(phys_addr_t pa)
{
	if (mmu_enabled) {
		clear_fixmap(FIX_PMD);
		return (pme_t *)set_fixmap_offset(FIX_PMD, pa);
	} else {
		return (pme_t *)((uintptr_t)pa);
	}
}

static phys_addr_t
alloc_pmd(uintptr_t va)
{
	uintptr_t pmd_num;

    BUG_ON(!mmu_enabled);
    BUG_ON(!phys_alloc_fn);
    return phys_alloc_fn(PAGE_SIZE, PAGE_SIZE);
}

static void
create_pmd_mapping(pme_t *pmdp,
                   uintptr_t va, phys_addr_t pa,
                   phys_addr_t sz, pgprot_t prot)
{
	uintptr_t pme_idx = pme_index(va);

    BUG_ON(sz != PME_SIZE);

    if (pme_none(pmdp[pme_idx]))
        pmdp[pme_idx] = pfn_pme(PFN_DOWN(pa), prot);
}

static void
create_pgd_mapping(pge_t *pgdp,
                   uintptr_t va, phys_addr_t pa,
                   phys_addr_t sz, pgprot_t prot)
{
	pme_t *nextp;
	phys_addr_t next_phys;
	uintptr_t pge_idx = pge_index(va);

	if (sz == PGE_SIZE) {
		if (pge_none(pgdp[pge_idx]))
			pgdp[pge_idx] = pfn_pge(PFN_DOWN(pa), prot);
		return;
	}

	if (pge_none(pgdp[pge_idx])) {
		next_phys = alloc_pmd(va);
		pgdp[pge_idx] = pfn_pge(PFN_DOWN(next_phys), PAGE_TABLE);
		nextp = get_pmd_virt(next_phys);
		memset(nextp, 0, PAGE_SIZE);
    } else {
		next_phys = PFN_PHYS(pge_pfn(pgdp[pge_idx]));
		nextp = get_pmd_virt(next_phys);
    }

	create_pmd_mapping(nextp, va, pa, sz, prot);
}

void
setup_vm_final(struct memblock_region *regions,
               unsigned long regions_cnt,
               phys_alloc_t alloc)
{
	uintptr_t va;
	phys_addr_t pa, start, end;
	struct memblock_region *reg;

	/* Set mmu_enabled flag */
	mmu_enabled = true;
    phys_alloc_fn = alloc;

	/* Setup swapper PGD for flash */
    setup_flash_pge();

	/* Setup swapper PGD for fixmap */
	create_pgd_mapping(swapper_pgd,
                       FIXADDR_START, (phys_addr_t)fixmap_pmd,
                       PGE_SIZE, PAGE_TABLE);

	/* Map all memory banks */
    for (reg = regions; reg < (regions + regions_cnt); reg++) {
        start = reg->base;
        end = start + reg->size;

        if (start >= end)
            break;

        for (pa = start; pa < end; pa += PME_SIZE) {
            va = (uintptr_t)__va(pa);
            create_pgd_mapping(swapper_pgd, va, pa,
                               PME_SIZE, PAGE_KERNEL_EXEC);
        }
    }

    /* Clear fixmap PTE and PMD mappings */
    clear_fixmap(FIX_PTE);
    clear_fixmap(FIX_PMD);

    /* Move to swapper page table */
    csr_write(CSR_SATP, PFN_DOWN(__pa(swapper_pgd)) | SATP_MODE);
    local_flush_tlb_all();
}
EXPORT_SYMBOL(setup_vm_final);
