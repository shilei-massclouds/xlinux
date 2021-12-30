// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
#include <errno.h>
#include <export.h>
#include <ptrace.h>
#include <pgalloc.h>
#include <pgtable.h>

/*
 * Allocate page middle directory.
 * We've already handled the fast-path in-line.
 */
int __pmd_alloc(struct mm_struct *mm, pgd_t *pgd, unsigned long address)
{
    pmd_t *new = pmd_alloc_one(mm, address);
    if (!new)
        return -ENOMEM;

    if (!pgd_present(*pgd)) {
        pgd_populate(mm, pgd, new);
    }

    return 0;
}

static vm_fault_t
__handle_mm_fault(struct vm_area_struct *vma,
                  unsigned long address, unsigned int flags)
{
    pgd_t *pgd;
    struct mm_struct *mm = vma->vm_mm;
    struct vm_fault vmf = {
        /*
        .vma = vma,
        .address = address & PAGE_MASK,
        .flags = flags,
        .pgoff = linear_page_index(vma, address),
        .gfp_mask = __get_fault_gfp_mask(vma),
        */
    };

    pgd = pgd_offset(mm, address);

    vmf.pmd = pmd_alloc(mm, pgd, address);
    if (!vmf.pmd)
        return VM_FAULT_OOM;

    panic("%s: !", __func__);
}

vm_fault_t
handle_mm_fault(struct vm_area_struct *vma, unsigned long address,
                unsigned int flags, struct pt_regs *regs)
{
    return __handle_mm_fault(vma, address, flags);
}
EXPORT_SYMBOL(handle_mm_fault);

static int
init_module(void)
{
    printk("module[pgalloc]: init begin ...\n");
    printk("module[pgalloc]: init end!\n");

    return 0;
}
