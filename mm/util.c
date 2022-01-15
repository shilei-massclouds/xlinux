// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
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

unsigned long
vm_mmap_pgoff(struct file *file, unsigned long addr,
              unsigned long len, unsigned long prot,
              unsigned long flag, unsigned long pgoff)
{
    unsigned long ret;
    unsigned long populate;
    LIST_HEAD(uf);

    ret = do_mmap(file, addr, len, prot, flag, pgoff, &populate, &uf);
    if (populate)
        mm_populate(ret, populate);

    return ret;
}

unsigned long vm_mmap(struct file *file, unsigned long addr,
                      unsigned long len, unsigned long prot,
                      unsigned long flag, unsigned long offset)
{
    if (unlikely(offset + PAGE_ALIGN(len) < offset))
        panic("bad arg!");
    if (unlikely(offset_in_page(offset)))
        panic("bad arg!");

    return vm_mmap_pgoff(file, addr, len, prot, flag, offset >> PAGE_SHIFT);
}
EXPORT_SYMBOL(vm_mmap);

