/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2009 Chen Liqin <liqin.chen@sunplusct.com>
 * Copyright (C) 2012 Regents of the University of California
 */

#ifndef _ASM_RISCV_PGALLOC_H
#define _ASM_RISCV_PGALLOC_H

#include <mm.h>
#include <string.h>
#include <pgtable.h>

static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
    pgd_t *pgd;

    pgd = (pgd_t *)__get_free_page(GFP_KERNEL);
    if (likely(pgd != NULL)) {
        memset(pgd, 0, USER_PTRS_PER_PGD * sizeof(pgd_t));
        /* Copy kernel mappings */
        memcpy(pgd + USER_PTRS_PER_PGD,
               init_mm.pgd + USER_PTRS_PER_PGD,
               (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
    }
    return pgd;
}

static inline pmd_t *
pmd_alloc_one(struct mm_struct *mm, unsigned long addr)
{
    struct page *page;
    gfp_t gfp = GFP_PGTABLE_USER;

    if (mm == &init_mm)
        gfp = GFP_PGTABLE_KERNEL;
    page = alloc_pages(gfp, 0);
    if (!page)
        return NULL;
    return (pmd_t *)page_address(page);
}

#endif /* _ASM_RISCV_PGALLOC_H */
