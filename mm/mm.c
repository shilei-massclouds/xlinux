// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
#include <bug.h>
#include <csr.h>
#include <export.h>
#include <fixmap.h>
#include <kernel.h>
#include <string.h>
#include <pgtable.h>
#include <mm_types.h>

static phys_alloc_t phys_alloc_fn;

void *dtb_early_va;
EXPORT_SYMBOL(dtb_early_va);

struct mm_struct init_mm = {
    .pgd    = swapper_pg_dir,
};
EXPORT_SYMBOL(init_mm);

void
setup_fixmap_pgd(void)
{
    uintptr_t va, end_va;
    uintptr_t pgd_idx = pgd_index(FIXADDR_START);
    uintptr_t pmd_idx = pmd_index(FIXADDR_START);

    BUG_ON(!pgd_none(early_pgd[pgd_idx]));
    BUG_ON(!pmd_none(fixmap_pmd[pmd_idx]));

    early_pgd[pgd_idx] =
        pfn_pgd(PFN_DOWN(__pa(fixmap_pmd)), PAGE_TABLE);

    fixmap_pmd[pmd_idx] =
        pfn_pmd(PFN_DOWN(__pa(fixmap_pt)), PAGE_TABLE);

    end_va = fix_to_virt(FIX_FDT) + FIX_FDT_SIZE;
    for (va = fix_to_virt(FIX_FDT); va < end_va; va += PAGE_SIZE) {
        uintptr_t pte_idx = pte_index(va);
        uintptr_t pa = dtb_early_pa + (va - fix_to_virt(FIX_FDT));
        fixmap_pt[pte_idx] = pfn_pte(PFN_DOWN(pa), PAGE_KERNEL);
    }

    dtb_early_va = (void *)fix_to_virt(FIX_FDT) + (dtb_early_pa & ~PAGE_MASK);
}
EXPORT_SYMBOL(setup_fixmap_pgd);

void
setup_flash_pgd(void)
{
    uintptr_t pgd_idx = pgd_index(FLASH_VA);
    BUG_ON(!pgd_none(swapper_pg_dir[pgd_idx]));
    swapper_pg_dir[pgd_idx] = pfn_pgd(PFN_DOWN(FLASH_PA), PAGE_KERNEL);
}

void
clear_flash_pgd(void)
{
    uintptr_t pgd_idx = pgd_index(FLASH_VA);
    swapper_pg_dir[pgd_idx] = __pgd(0);
}
EXPORT_SYMBOL(clear_flash_pgd);

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

static pmd_t *
get_pmd_virt(phys_addr_t pa)
{
    clear_fixmap(FIX_PMD);
    return (pmd_t *)set_fixmap_offset(FIX_PMD, pa);
}

static phys_addr_t
alloc_pmd(uintptr_t va)
{
	uintptr_t pmd_num;

    BUG_ON(!phys_alloc_fn);
    return phys_alloc_fn(PAGE_SIZE, PAGE_SIZE);
}

static void
create_pmd_mapping(pmd_t *pmdp,
                   uintptr_t va, phys_addr_t pa,
                   phys_addr_t sz, pgprot_t prot)
{
	uintptr_t pmd_idx = pmd_index(va);

    BUG_ON(sz != PMD_SIZE);

    if (pmd_none(pmdp[pmd_idx]))
        pmdp[pmd_idx] = pfn_pmd(PFN_DOWN(pa), prot);
}

static void
create_pgd_mapping(pgd_t *pgdp,
                   uintptr_t va, phys_addr_t pa,
                   phys_addr_t sz, pgprot_t prot)
{
	pmd_t *nextp;
	phys_addr_t next_phys;
	uintptr_t pgd_idx = pgd_index(va);

	if (sz == PGD_SIZE) {
		if (pgd_none(pgdp[pgd_idx]))
			pgdp[pgd_idx] = pfn_pgd(PFN_DOWN(pa), prot);
		return;
	}

	if (pgd_none(pgdp[pgd_idx])) {
		next_phys = alloc_pmd(va);
		pgdp[pgd_idx] = pfn_pgd(PFN_DOWN(next_phys), PAGE_TABLE);
		nextp = get_pmd_virt(next_phys);
		memset(nextp, 0, PAGE_SIZE);
    } else {
		next_phys = PFN_PHYS(pgd_pfn(pgdp[pgd_idx]));
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

    phys_alloc_fn = alloc;

	/* Setup swapper PGD for flash */
    setup_flash_pgd();

	/* Setup swapper PGD for fixmap */
	create_pgd_mapping(swapper_pg_dir,
                       FIXADDR_START, __pa(fixmap_pmd),
                       PGD_SIZE, PAGE_TABLE);

	/* Map all memory banks */
    for (reg = regions; reg < (regions + regions_cnt); reg++) {
        start = reg->base;
        end = start + reg->size;

        if (start >= end)
            break;

        for (pa = start; pa < end; pa += PMD_SIZE) {
            va = (uintptr_t)__va(pa);
            create_pgd_mapping(swapper_pg_dir, va, pa,
                               PMD_SIZE, PAGE_KERNEL_EXEC);
        }
    }

    /* Clear fixmap PTE and PMD mappings */
    clear_fixmap(FIX_PTE);
    clear_fixmap(FIX_PMD);

    /* Move to swapper page table */
    csr_write(CSR_SATP, PFN_DOWN(__pa(swapper_pg_dir)) | SATP_MODE);
    local_flush_tlb_all();
}
EXPORT_SYMBOL(setup_vm_final);

static int
init_module(void)
{
    printk("module[mm]: init begin ...\n");
    clear_flash_pgd();
    setup_fixmap_pgd();
    printk("module[mm]: init end!\n");

    return 0;
}
