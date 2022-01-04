/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_HIGHMEM_H
#define _LINUX_HIGHMEM_H

#include <mm_types.h>

static inline struct page *
__alloc_zeroed_user_highpage(gfp_t movableflags,
                             struct vm_area_struct *vma,
                             unsigned long vaddr)
{
    struct page *page = alloc_page_vma(GFP_HIGHUSER | movableflags,
                                       vma, vaddr);

    return page;
}

/**
 * alloc_zeroed_user_highpage_movable - Allocate a zeroed HIGHMEM page
 * for a VMA that the caller knows can move
 * @vma: The VMA the page is to be allocated for
 * @vaddr: The virtual address the page will be inserted into
 *
 * This function will allocate a page for a VMA that the caller knows
 * will be able to migrate in the future using move_pages() or reclaimed
 */
static inline struct page *
alloc_zeroed_user_highpage_movable(struct vm_area_struct *vma,
                                   unsigned long vaddr)
{
    return __alloc_zeroed_user_highpage(__GFP_MOVABLE, vma, vaddr);
}

static inline void *kmap_atomic(struct page *page)
{
    return page_address(page);
}
#define kmap_atomic_prot(page, prot)    kmap_atomic(page)

#endif /* _LINUX_HIGHMEM_H */
