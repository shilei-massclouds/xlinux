/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _PAGE_H
#define _PAGE_H

#include <config.h>
#include <const.h>
#include <types.h>
#include <pgtable-bits.h>

#define PAGE_SHIFT	(12)
#define PAGE_SIZE	(_AC(1, UL) << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))

#define PAGE_OFFSET _AC(CONFIG_PAGE_OFFSET, UL)

#define KERN_VIRT_SIZE (-PAGE_OFFSET)

#define MAX_ORDER 11

/*
 * paging: SV-39, va-39bits, pa-56bits
 * root -> pgd(pge) -> pmd(pme) -> pt(pte) -> phy_page
 * root is the base of pgd;
 * pgd is a table of 512 pge which points to a pmd;
 * pmd is a table of 512 pme which points to a pt;
 * pt is a table of 512 pte which points to physical page.
 */

#define PGE_SHIFT       (30)
#define PGE_SIZE        (_AC(1, UL) << PGE_SHIFT)
#define PGE_MASK        (~(PGE_SIZE - 1))

/* Number of pge in a pgd */
#define PTRS_PER_PGD    (PAGE_SIZE / sizeof(pge_t))
#define pge_index(a)    (((a) >> PGE_SHIFT) & (PTRS_PER_PGD - 1))
#define pge_val(x)      ((x).pge)
#define __pge(x)        ((pge_t) { (x) })

#define PME_SHIFT       (21)
#define PME_SIZE        (_AC(1, UL) << PME_SHIFT)
#define PME_MASK        (~(PME_SIZE - 1))

/* Number of pme in a pmd */
#define PTRS_PER_PMD    (PAGE_SIZE / sizeof(pme_t))
#define pme_index(a)    (((a) >> PME_SHIFT) & (PTRS_PER_PMD - 1))
#define pme_val(x)      ((x).pme)
#define __pme(x)        ((pme_t) { (x) })

/* Number of pte in a pt */
#define PTRS_PER_PT     (PAGE_SIZE / sizeof(pte_t))
#define pte_index(a)    (((a) >> PAGE_SHIFT) & (PTRS_PER_PT - 1))
#define pte_val(x)      ((x).pte)
#define __pte(x)        ((pte_t) { (x) })

#define pgprot_val(x)   ((x).pgprot)
#define __pgprot(x)     ((pgprot_t) { (x) })

#define PFN_ALIGN(x)    (((unsigned long)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)
#define PFN_UP(x)       (((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(x)     ((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)     ((phys_addr_t)(x) << PAGE_SHIFT)
#define PHYS_PFN(x)     ((unsigned long)((x) >> PAGE_SHIFT))

#define pfn_pge(pfn, prot) \
    __pge(((pfn) << _PAGE_PFN_SHIFT) | pgprot_val((prot)))

#define pge_pfn(pge) \
    (pge_val(pge) >> _PAGE_PFN_SHIFT)

#define pfn_pme(pfn, prot) \
    __pme(((pfn) << _PAGE_PFN_SHIFT) | pgprot_val((prot)))

#define pfn_pte(pfn, prot) \
    __pte(((pfn) << _PAGE_PFN_SHIFT) | pgprot_val((prot)))

#define _PAGE_BASE  (_PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_USER)

#define PAGE_NONE       __pgprot(_PAGE_PROT_NONE)
#define PAGE_READ       __pgprot(_PAGE_BASE | _PAGE_READ)
#define PAGE_WRITE      __pgprot(_PAGE_BASE | _PAGE_READ | _PAGE_WRITE)
#define PAGE_EXEC       __pgprot(_PAGE_BASE | _PAGE_EXEC)
#define PAGE_READ_EXEC  __pgprot(_PAGE_BASE | _PAGE_READ | _PAGE_EXEC)
#define PAGE_WRITE_EXEC __pgprot(_PAGE_BASE | _PAGE_READ | \
                                 _PAGE_EXEC | _PAGE_WRITE)

#define _PAGE_KERNEL    (_PAGE_READ | _PAGE_WRITE | \
                         _PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_DIRTY)

#define PAGE_KERNEL         __pgprot(_PAGE_KERNEL)
#define PAGE_KERNEL_EXEC    __pgprot(_PAGE_KERNEL | _PAGE_EXEC)
#define PAGE_TABLE          __pgprot(_PAGE_TABLE)

#define FIXMAP_PAGE_NORMAL  PAGE_KERNEL

#define __va_to_pa(x)       ((unsigned long)(x) - va_pa_offset)
#define __virt_to_phys(x)   __va_to_pa(x)
#define __pa(x)             __virt_to_phys((unsigned long)(x))
#define virt_to_phys(x)     __pa((unsigned long)x)

#define __pa_to_va(x)   ((void *)((unsigned long) (x) + va_pa_offset))
#define __va(x)         ((void *)__pa_to_va((phys_addr_t)(x)))
#define phys_to_virt(x) __va(x)

#ifndef __ASSEMBLY__

#include <list.h>

extern unsigned long va_pa_offset;

typedef struct {
    unsigned long pgprot;
} pgprot_t;

typedef struct {
    unsigned long pge;
} pge_t;

typedef struct {
    unsigned long pme;
} pme_t;

typedef struct {
    unsigned long pte;
} pte_t;

typedef struct {
    u64 val;
} pfn_t;

typedef struct page *pgtable_t;

struct page {
    /* slab */
    struct {
        struct list_head slab_list;
        void *freelist; /* first free object */
        void *s_mem;    /* first object */
    };

    unsigned int active;
};

#include <log2.h>
#define STRUCT_PAGE_MAX_SHIFT   (order_base_2(sizeof(struct page)))

#endif /* !__ASSEMBLY__ */

#ifndef pte_none
#define pte_none(pte) (!pte_val(pte))
#endif

#ifndef pme_none
#define pme_none(pme) (!pme_val(pme))
#endif

#ifndef pge_none
#define pge_none(pge) (!pge_val(pge))
#endif

/* test whether an address (unsigned long or pointer) is aligned to PAGE_SIZE */
#define PAGE_ALIGNED(addr)  IS_ALIGNED((unsigned long)(addr), PAGE_SIZE)
#define PAGE_ALIGN(addr)    _ALIGN(addr, PAGE_SIZE)

#endif /* _PAGE_H */
