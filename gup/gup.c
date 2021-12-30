// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
#include <bug.h>
#include <errno.h>
#include <types.h>
#include <export.h>
#include <pgtable.h>

static struct page *
no_page_table(struct vm_area_struct *vma, unsigned int flags)
{
    return NULL;
}

static struct page *
follow_pmd_mask(struct vm_area_struct *vma,
                unsigned long address, pgd_t *pgdp,
                unsigned int flags)
{
    panic("%s: !", __func__);
}

static struct page *
follow_page_mask(struct vm_area_struct *vma,
                 unsigned long address, unsigned int flags)
{
    pgd_t *pgd;
    struct mm_struct *mm = vma->vm_mm;

    pgd = pgd_offset(mm, address);

    if (pgd_none(*pgd))
        return no_page_table(vma, flags);

    return follow_pmd_mask(vma, address, pgd, flags);
}

static int faultin_page(struct vm_area_struct *vma,
                        unsigned long address,
                        unsigned int *flags, int *locked)
{
    vm_fault_t ret;
    unsigned int fault_flags = 0;

    ret = handle_mm_fault(vma, address, fault_flags, NULL);
    if (ret & VM_FAULT_ERROR)
        panic("handle mm fault error!");

    panic("%s: !", __func__);
}

static long
__get_user_pages(struct mm_struct *mm,
                 unsigned long start, unsigned long nr_pages,
                 unsigned int gup_flags, struct page **pages,
                 struct vm_area_struct **vmas, int *locked)
{
    long ret = 0, i = 0;
    struct vm_area_struct *vma = NULL;

    if (!nr_pages)
        return 0;

    start = untagged_addr(start);

    BUG_ON(!!pages != !!(gup_flags & (FOLL_GET | FOLL_PIN)));

    do {
        struct page *page;
        unsigned int foll_flags = gup_flags;
        unsigned int page_increm;

        /* first iteration or cross vma bound */
        if (!vma || start >= vma->vm_end) {
            vma = find_extend_vma(mm, start);
            if (!vma)
                panic("find error!");
        }

 retry:
        page = follow_page_mask(vma, start, foll_flags);
        if (!page) {
            ret = faultin_page(vma, start, &foll_flags, locked);
            switch (ret) {
            case 0:
                goto retry;
            default:
                panic("can not faultin page!");
            }
        } else if (PTR_ERR(page) == -EEXIST) {
            /*
             * Proper page table entry exists, but no corresponding
             * struct page.
             */
            goto next_page;
        } else if (IS_ERR(page)) {
            panic("follow page error: %d", PTR_ERR(page));
        }

 next_page:
        panic("%s: page(%lx)", __func__, page);
    } while (nr_pages);

    panic("%s: !", __func__);
}

static inline long
__get_user_pages_locked(struct mm_struct *mm,
                        unsigned long start,
                        unsigned long nr_pages,
                        struct page **pages,
                        struct vm_area_struct **vmas,
                        int *locked,
                        unsigned int flags)
{
    long ret, pages_done;
    bool lock_dropped;

    if (locked) {
        /* if VM_FAULT_RETRY can be returned, vmas become invalid */
        BUG_ON(vmas);
        /* check caller initialized locked */
        BUG_ON(*locked != 1);
    }

    /*
     * FOLL_PIN and FOLL_GET are mutually exclusive. Traditional behavior
     * is to set FOLL_GET if the caller wants pages[] filled in (but has
     * carelessly failed to specify FOLL_GET), so keep doing that, but only
     * for FOLL_GET, not for the newer FOLL_PIN.
     *
     * FOLL_PIN always expects pages to be non-null, but no need to assert
     * that here, as any failures will be obvious enough.
     */
    if (pages && !(flags & FOLL_PIN))
        flags |= FOLL_GET;

    pages_done = 0;
    lock_dropped = false;
    for (;;) {
        ret = __get_user_pages(mm, start, nr_pages, flags, pages,
                               vmas, locked);
        if (!locked)
            /* VM_FAULT_RETRY couldn't trigger, bypass */
            return ret;

        panic("%s: 1", __func__);
    }

    panic("%s: !", __func__);
}

static long
__get_user_pages_remote(struct mm_struct *mm,
                        unsigned long start, unsigned long nr_pages,
                        unsigned int gup_flags, struct page **pages,
                        struct vm_area_struct **vmas, int *locked)
{
    /*
     * Parts of FOLL_LONGTERM behavior are incompatible with
     * FAULT_FLAG_ALLOW_RETRY because of the FS DAX check requirement on
     * vmas. However, this only comes up if locked is set, and there are
     * callers that do request FOLL_LONGTERM, but do not set locked. So,
     * allow what we can.
     */
    if (gup_flags & FOLL_LONGTERM)
        panic("not support FOLL_LONGTERM!");

    return __get_user_pages_locked(mm, start, nr_pages, pages, vmas,
                                   locked,
                                   gup_flags | FOLL_TOUCH | FOLL_REMOTE);
}

long get_user_pages_remote(struct mm_struct *mm,
                           unsigned long start, unsigned long nr_pages,
                           unsigned int gup_flags, struct page **pages,
                           struct vm_area_struct **vmas, int *locked)
{
    /*
     * FOLL_PIN must only be set internally by the pin_user_pages*() APIs,
     * never directly by the caller, so enforce that with an assertion:
     */
    BUG_ON(gup_flags & FOLL_PIN);

    return __get_user_pages_remote(mm, start, nr_pages, gup_flags,
                                   pages, vmas, locked);
}
EXPORT_SYMBOL(get_user_pages_remote);

static int
init_module(void)
{
    printk("module[gup]: init begin ...\n");
    printk("module[gup]: init end!\n");

    return 0;
}
