// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
#include <errno.h>
#include <export.h>
#include <ptrace.h>
#include <highmem.h>
#include <pagemap.h>
#include <pgalloc.h>
#include <pgtable.h>

static unsigned long fault_around_bytes = rounddown_pow_of_two(65536);

/*
 * Allocate page middle directory.
 * We've already handled the fast-path in-line.
 */
int __pmd_alloc(struct mm_struct *mm, pgd_t *pgd, unsigned long address)
{
    pmd_t *new = pmd_alloc_one(mm, address);
    if (!new)
        return -ENOMEM;

    if (!pgd_present(*pgd))
        pgd_populate(mm, pgd, new);
    return 0;
}
EXPORT_SYMBOL(__pmd_alloc);

int __pte_alloc(struct mm_struct *mm, pmd_t *pmd)
{
    pgtable_t new = pte_alloc_one(mm);
    if (!new)
        return -ENOMEM;

    if (likely(pmd_none(*pmd))) {   /* Has another populated it ? */
        pmd_populate(mm, pmd, new);
        new = NULL;
    }
    if (new)
        panic("set pte into pmd error!");
    return 0;
}

static vm_fault_t do_anonymous_page(struct vm_fault *vmf)
{
    pte_t entry;
    struct page *page;
    struct vm_area_struct *vma = vmf->vma;

    if (pte_alloc(vma->vm_mm, vmf->pmd))
        return VM_FAULT_OOM;

    page = alloc_zeroed_user_highpage_movable(vma, vmf->address);
    if (!page)
        panic("out of memory!");

    __SetPageUptodate(page);

    entry = mk_pte(page, vma->vm_page_prot);
    entry = pte_sw_mkyoung(entry);

    vmf->pte = pte_offset_map_lock(vma->vm_mm, vmf->pmd, vmf->address);
    if (!pte_none(*vmf->pte))
        panic("bad pte!");

    set_pte_at(vma->vm_mm, vmf->address, vmf->pte, entry);
    return 0;
}

static vm_fault_t do_fault_around(struct vm_fault *vmf)
{
    int off;
    pgoff_t end_pgoff;
    vm_fault_t ret = 0;
    pgoff_t start_pgoff = vmf->pgoff;
    unsigned long address = vmf->address, nr_pages, mask;

    nr_pages = READ_ONCE(fault_around_bytes) >> PAGE_SHIFT;
    mask = ~(nr_pages * PAGE_SIZE - 1) & PAGE_MASK;

    vmf->address = max(address & mask, vmf->vma->vm_start);
    off = ((address - vmf->address) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1);
    start_pgoff -= off;

    /*
     *  end_pgoff is either the end of the page table, the end of
     *  the vma or nr_pages from start_pgoff, depending what is nearest.
     */
    end_pgoff = start_pgoff -
        ((vmf->address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1)) +
        PTRS_PER_PTE - 1;
    end_pgoff = min3(end_pgoff,
                     vma_pages(vmf->vma) + vmf->vma->vm_pgoff - 1,
                     start_pgoff + nr_pages - 1);

    if (pmd_none(*vmf->pmd)) {
        vmf->prealloc_pte = pte_alloc_one(vmf->vma->vm_mm);
        if (!vmf->prealloc_pte)
            panic("bad memory!");
    }

    vmf->vma->vm_ops->map_pages(vmf, start_pgoff, end_pgoff);

    /* ->map_pages() haven't done anything useful. Cold page cache? */
    if (!vmf->pte)
        panic("no pte!");

    /* check if the page fault is solved */
    vmf->pte -= (vmf->address >> PAGE_SHIFT) - (address >> PAGE_SHIFT);
    if (!pte_none(*vmf->pte))
        ret = VM_FAULT_NOPAGE;

    vmf->address = address;
    vmf->pte = NULL;
    return ret;
}

static vm_fault_t do_read_fault(struct vm_fault *vmf)
{
    vm_fault_t ret = 0;
    struct vm_area_struct *vma = vmf->vma;

    /*
     * Let's call ->map_pages() first and use ->fault() as fallback
     * if page by the offset is not ready to be mapped (cold cache or
     * something).
     */
    if (vma->vm_ops->map_pages && fault_around_bytes >> PAGE_SHIFT > 1) {
        ret = do_fault_around(vmf);
        if (ret)
            return ret;
    }

    panic("%s: !", __func__);
}

static vm_fault_t __do_fault(struct vm_fault *vmf)
{
    vm_fault_t ret;
    struct vm_area_struct *vma = vmf->vma;

    if (pmd_none(*vmf->pmd) && !vmf->prealloc_pte) {
        vmf->prealloc_pte = pte_alloc_one(vmf->vma->vm_mm);
        if (!vmf->prealloc_pte)
            return VM_FAULT_OOM;
    }

    ret = vma->vm_ops->fault(vmf);
    if (unlikely(ret & (VM_FAULT_ERROR | VM_FAULT_NOPAGE | VM_FAULT_RETRY |
                        VM_FAULT_DONE_COW)))
        return ret;

    panic("%s: !", __func__);
}

static vm_fault_t do_cow_fault(struct vm_fault *vmf)
{
    vm_fault_t ret;
    struct vm_area_struct *vma = vmf->vma;

    vmf->cow_page = alloc_page_vma(GFP_HIGHUSER_MOVABLE, vma, vmf->address);
    if (!vmf->cow_page)
        return VM_FAULT_OOM;

    ret = __do_fault(vmf);
    if (unlikely(ret & (VM_FAULT_ERROR | VM_FAULT_NOPAGE | VM_FAULT_RETRY)))
        panic("bad fault!");
    if (ret & VM_FAULT_DONE_COW)
        return ret;

    panic("%s: !", __func__);
}

static vm_fault_t do_fault(struct vm_fault *vmf)
{
    vm_fault_t ret;
    struct vm_area_struct *vma = vmf->vma;

    if (!vma->vm_ops->fault) {
        panic("no fault func!");
    } else if (!(vmf->flags & FAULT_FLAG_WRITE)) {
        ret = do_read_fault(vmf);
    } else if (!(vma->vm_flags & VM_SHARED)) {
        ret = do_cow_fault(vmf);
    } else {
        panic("shared fault!");
    }

    /* preallocated pagetable is unused: free it */
    if (vmf->prealloc_pte) {
        vmf->prealloc_pte = NULL;
    }
    return ret;
}

static vm_fault_t handle_pte_fault(struct vm_fault *vmf)
{
    if (unlikely(pmd_none(*vmf->pmd))) {
        /*
         * Leave __pte_alloc() until later: because vm_ops->fault may
         * want to allocate huge page, and if we expose page table
         * for an instant, it will be difficult to retract from
         * concurrent faults and from rmap lookups.
         */
        vmf->pte = NULL;
    } else {
        /*
         * A regular pmd is established and it can't morph into a huge
         * pmd from under us anymore at this point because we hold the
         * mmap_lock read mode and khugepaged takes it in write mode.
         * So now it's safe to run pte_offset_map().
         */
        vmf->pte = pte_offset_map(vmf->pmd, vmf->address);
        vmf->orig_pte = *vmf->pte;

        /*
         * some architectures can have larger ptes than wordsize,
         * e.g.ppc44x-defconfig has CONFIG_PTE_64BIT=y and
         * CONFIG_32BIT=y, so READ_ONCE cannot guarantee atomic
         * accesses.  The code below just needs a consistent view
         * for the ifs and we later double check anyway with the
         * ptl lock held. So here a barrier will do.
         */
        if (pte_none(vmf->orig_pte))
            vmf->pte = NULL;
    }

    if (!vmf->pte) {
        if (vma_is_anonymous(vmf->vma))
            return do_anonymous_page(vmf);
        else
            return do_fault(vmf);
    }

    panic("%s: !", __func__);
}

static vm_fault_t
__handle_mm_fault(struct vm_area_struct *vma,
                  unsigned long address, unsigned int flags)
{
    pgd_t *pgd;
    struct mm_struct *mm = vma->vm_mm;
    struct vm_fault vmf = {
        .vma = vma,
        .address = address & PAGE_MASK,
        .flags = flags,
        .pgoff = linear_page_index(vma, address),
    };

    pgd = pgd_offset(mm, address);

    vmf.pmd = pmd_alloc(mm, pgd, address);
    if (!vmf.pmd)
        return VM_FAULT_OOM;
    return handle_pte_fault(&vmf);
}

vm_fault_t
_handle_mm_fault(struct vm_area_struct *vma, unsigned long address,
                 unsigned int flags, struct pt_regs *regs)
{
    return __handle_mm_fault(vma, address, flags);
}

struct page *
vm_normal_page(struct vm_area_struct *vma, unsigned long addr,
               pte_t pte)
{
    unsigned long pfn = pte_pfn(pte);

    return pfn_to_page(pfn);
}
EXPORT_SYMBOL(vm_normal_page);

static vm_fault_t pte_alloc_one_map(struct vm_fault *vmf)
{
    struct vm_area_struct *vma = vmf->vma;

    if (!pmd_none(*vmf->pmd))
        goto map_pte;
    if (vmf->prealloc_pte) {
        if (unlikely(!pmd_none(*vmf->pmd)))
            goto map_pte;

        pmd_populate(vma->vm_mm, vmf->pmd, vmf->prealloc_pte);
        vmf->prealloc_pte = NULL;
    } else if (unlikely(pte_alloc(vma->vm_mm, vmf->pmd))) {
        return VM_FAULT_OOM;
    }

 map_pte:

    vmf->pte = pte_offset_map_lock(vma->vm_mm, vmf->pmd, vmf->address);
    return 0;
}

void page_add_file_rmap(struct page *page, bool compound)
{
    /* Todo: */
}

vm_fault_t alloc_set_pte(struct vm_fault *vmf, struct page *page)
{
    pte_t entry;
    vm_fault_t ret;
    struct vm_area_struct *vma = vmf->vma;
    bool write = vmf->flags & FAULT_FLAG_WRITE;

    if (!vmf->pte) {
        ret = pte_alloc_one_map(vmf);
        if (ret)
            return ret;
    }

    if (unlikely(!pte_none(*vmf->pte)))
        return VM_FAULT_NOPAGE;

    entry = mk_pte(page, vma->vm_page_prot);
    entry = pte_sw_mkyoung(entry);
    if (write)
        panic("fault write!");
    /* copy-on-write page */
    if (write && !(vma->vm_flags & VM_SHARED)) {
        panic("fault write!");
    } else {
        page_add_file_rmap(page, false);
    }
    set_pte_at(vma->vm_mm, vmf->address, vmf->pte, entry);
    return 0;
}
EXPORT_SYMBOL(alloc_set_pte);

static int
init_module(void)
{
    printk("module[pgalloc]: init begin ...\n");

    handle_mm_fault = _handle_mm_fault;

    printk("module[pgalloc]: init end!\n");

    return 0;
}
