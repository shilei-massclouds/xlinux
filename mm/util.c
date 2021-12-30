// SPDX-License-Identifier: GPL-2.0-only

#include <export.h>
#include <mm_types.h>

void __vma_link_list(struct mm_struct *mm, struct vm_area_struct *vma,
                     struct vm_area_struct *prev)
{
    struct vm_area_struct *next;

    vma->vm_prev = prev;
    if (prev) {
        next = prev->vm_next;
        prev->vm_next = vma;
    } else {
        next = mm->mmap;
        mm->mmap = vma;
    }
    vma->vm_next = next;
    if (next)
        next->vm_prev = vma;
}
EXPORT_SYMBOL(__vma_link_list);
