/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _RISCV_MM_H_
#define _RISCV_MM_H_

#include <page.h>
#include <memblock.h>

extern pge_t early_pgd[];
extern pme_t early_pmd[];
extern pme_t fixmap_pmd[];
extern pte_t fixmap_pt[];
extern pge_t swapper_pgd[];

extern phys_addr_t dtb_early_pa;

typedef phys_addr_t (*phys_alloc_t)(phys_addr_t size, phys_addr_t align);

void
setup_fixmap_pge(void);

void
setup_vm_final(struct memblock_region *regions,
               unsigned long regions_cnt,
               phys_alloc_t alloc);

void
clear_flash_pge(void);

const char *
kstrdup_const(const char *s);

char *
kstrdup(const char *s);

void
kfree_const(const void *x);

#endif /* _RISCV_MM_H_ */
