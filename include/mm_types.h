/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MM_TYPES_H
#define _LINUX_MM_TYPES_H

#include <page.h>
#include <rbtree.h>

/* NEW_AUX_ENT entries in auxiliary table */
#define AT_VECTOR_SIZE_BASE 20  /* from "include/linux/auxvec.h" */
#define AT_VECTOR_SIZE (2 * (AT_VECTOR_SIZE_BASE + 1))

#define page_private(page)  ((page)->private)

/**
 * typedef vm_fault_t - Return type for page fault handlers.
 *
 * Page fault handlers return a bitmask of %VM_FAULT values.
 */
typedef unsigned int vm_fault_t;

typedef unsigned long vm_flags_t;

enum vm_fault_reason {
    VM_FAULT_OOM            = (__force vm_fault_t)0x000001,
    VM_FAULT_SIGBUS         = (__force vm_fault_t)0x000002,
    VM_FAULT_MAJOR          = (__force vm_fault_t)0x000004,
    VM_FAULT_WRITE          = (__force vm_fault_t)0x000008,
    VM_FAULT_HWPOISON       = (__force vm_fault_t)0x000010,
    VM_FAULT_HWPOISON_LARGE = (__force vm_fault_t)0x000020,
    VM_FAULT_SIGSEGV        = (__force vm_fault_t)0x000040,
    VM_FAULT_NOPAGE         = (__force vm_fault_t)0x000100,
    VM_FAULT_LOCKED         = (__force vm_fault_t)0x000200,
    VM_FAULT_RETRY          = (__force vm_fault_t)0x000400,
    VM_FAULT_FALLBACK       = (__force vm_fault_t)0x000800,
    VM_FAULT_DONE_COW       = (__force vm_fault_t)0x001000,
    VM_FAULT_NEEDDSYNC      = (__force vm_fault_t)0x002000,
    VM_FAULT_HINDEX_MASK    = (__force vm_fault_t)0x0f0000,
};

#define VM_FAULT_ERROR (VM_FAULT_OOM | VM_FAULT_SIGBUS | \
                        VM_FAULT_SIGSEGV | VM_FAULT_HWPOISON | \
                        VM_FAULT_HWPOISON_LARGE | VM_FAULT_FALLBACK)

struct mm_struct {
    struct vm_area_struct *mmap;    /* list of VMAs */
    struct rb_root mm_rb;
    pgd_t *pgd;
    unsigned long saved_auxv[AT_VECTOR_SIZE]; /* for /proc/PID/auxv */
    unsigned long total_vm;     /* Total pages mapped */
    unsigned long stack_vm;     /* VM_STACK */
    unsigned long data_vm;      /* VM_WRITE & ~VM_SHARED & ~VM_STACK */

    unsigned long mmap_base;    /* base of mmap area */

    /* store ref to file /proc/<pid>/exe symlink points to */
    struct file *exe_file;

    unsigned long task_size;        /* size of task vm space */
    unsigned long start_code, end_code, start_data, end_data;
    unsigned long start_brk, brk, start_stack;
    unsigned long arg_start, arg_end, env_start, env_end;
    unsigned long def_flags;

    struct linux_binfmt *binfmt;

    unsigned long highest_vm_end;   /* highest vma end address */

    unsigned long (*get_unmapped_area)(struct file *filp,
                                       unsigned long addr,
                                       unsigned long len,
                                       unsigned long pgoff,
                                       unsigned long flags);
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

    pgprot_t vm_page_prot;
    unsigned long vm_flags;     /* Flags, see mm.h. */

    unsigned long vm_pgoff;     /* Offset within vm_file in PAGE_SIZE units */

    struct file *vm_file;       /* File we map to (can be NULL). */

    /* Function pointers to deal with this struct. */
    const struct vm_operations_struct *vm_ops;
};

static inline void
set_page_private(struct page *page, unsigned long private)
{
    page->private = private;
}

#endif /* _LINUX_MM_TYPES_H */
