/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MM_TYPES_H
#define _LINUX_MM_TYPES_H

#include <page.h>
#include <rbtree.h>

/* NEW_AUX_ENT entries in auxiliary table */
#define AT_VECTOR_SIZE_BASE 20  /* from "include/linux/auxvec.h" */
#define AT_VECTOR_SIZE (2 * (AT_VECTOR_SIZE_BASE + 1))

#define page_private(page)  ((page)->private)

struct mm_struct {
    struct vm_area_struct *mmap;    /* list of VMAs */
    struct rb_root mm_rb;
    pgd_t *pgd;
    unsigned long saved_auxv[AT_VECTOR_SIZE]; /* for /proc/PID/auxv */
    unsigned long total_vm;    /* Total pages mapped */
    unsigned long stack_vm;    /* VM_STACK */

    unsigned long highest_vm_end;   /* highest vma end address */
};

struct vm_operations_struct {
};

struct vm_area_struct {
    unsigned long vm_start; /* Our start address within vm_mm. */
    unsigned long vm_end;   /* The first byte after our end address
                               within vm_mm. */

    /* linked list of VM areas per task, sorted by address */
    struct vm_area_struct *vm_next, *vm_prev;

    struct rb_node vm_rb;

    /*
     * Largest free memory gap in bytes to the left of this VMA.
     * Either between this VMA and vma->vm_prev, or between one of the
     * VMAs below us in the VMA rbtree and its ->vm_prev. This helps
     * get_unmapped_area find a free area of the right size.
     */
    unsigned long rb_subtree_gap;

    struct mm_struct *vm_mm;    /* The address space we belong to. */

    unsigned long vm_flags;     /* Flags, see mm.h. */

    unsigned long vm_pgoff;     /* Offset within vm_file in PAGE_SIZE units */

    /* Function pointers to deal with this struct. */
    const struct vm_operations_struct *vm_ops;
};

static inline void
set_page_private(struct page *page, unsigned long private)
{
    page->private = private;
}

#endif /* _LINUX_MM_TYPES_H */
