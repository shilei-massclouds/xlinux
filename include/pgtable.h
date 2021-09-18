/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PGTABLE_H
#define _LINUX_PGTABLE_H

#include <const.h>
#include <page.h>
#include <sizes.h>

#define VMALLOC_SIZE    (KERN_VIRT_SIZE >> 1)
#define VMALLOC_END     (PAGE_OFFSET - 1)
#define VMALLOC_START   (PAGE_OFFSET - VMALLOC_SIZE)

#define VMEMMAP_SHIFT \
    (CONFIG_VA_BITS - PAGE_SHIFT - 1 + STRUCT_PAGE_MAX_SHIFT)

#define VMEMMAP_SIZE    (_AC(1, UL) << VMEMMAP_SHIFT)
#define VMEMMAP_END     (VMALLOC_START - 1)
#define VMEMMAP_START   (VMALLOC_START - VMEMMAP_SIZE)

#define PCI_IO_SIZE     SZ_16M
#define PCI_IO_END      VMEMMAP_START
#define PCI_IO_START    (PCI_IO_END - PCI_IO_SIZE)

#define FIXADDR_SIZE    PME_SIZE
#define FIXADDR_TOP     PCI_IO_START
#define FIXADDR_START   (FIXADDR_TOP - FIXADDR_SIZE)

/*
 * Address for flash
 * The flash is used for module
 */
#define FLASH_VA        0xFFFFFFCF20000000UL
//#define FLASH_VA        0xFFFFFFCF80000000UL
//#define FLASH_VA        ((FIXADDR_START & PGE_MASK) - PGE_SIZE)
#define FLASH_PA        0x0000000020000000UL
#define FLASH_HEAD_SIZE 0x100

static inline void
set_pte(pte_t *ptep, pte_t pteval)
{
    *ptep = pteval;
}

#endif /* _LINUX_PGTABLE_H */
