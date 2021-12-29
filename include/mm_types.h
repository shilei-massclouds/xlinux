/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MM_TYPES_H
#define _LINUX_MM_TYPES_H

#include <page.h>

/* NEW_AUX_ENT entries in auxiliary table */
#define AT_VECTOR_SIZE_BASE 20  /* from "include/linux/auxvec.h" */
#define AT_VECTOR_SIZE (2 * (AT_VECTOR_SIZE_BASE + 1))

#define page_private(page)  ((page)->private)

struct mm_struct {
    pgd_t *pgd;
    unsigned long saved_auxv[AT_VECTOR_SIZE]; /* for /proc/PID/auxv */
    unsigned long total_vm;    /* Total pages mapped */
    unsigned long stack_vm;    /* VM_STACK */
};

struct vm_operations_struct {
};

struct vm_area_struct {
    unsigned long vm_start; /* Our start address within vm_mm. */
    unsigned long vm_end;   /* The first byte after our end address
                               within vm_mm. */

    struct mm_struct *vm_mm;    /* The address space we belong to. */

    /* Function pointers to deal with this struct. */
    const struct vm_operations_struct *vm_ops;
};

static inline void
set_page_private(struct page *page, unsigned long private)
{
    page->private = private;
}

#endif /* _LINUX_MM_TYPES_H */
