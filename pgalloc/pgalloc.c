// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
#include <errno.h>
#include <export.h>
#include <ptrace.h>
#include <highmem.h>
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

    printk("%s: 1\n", __func__);
    if (!pgd_present(*pgd)) {
        printk("%s: 1.5\n", __func__);
        pgd_populate(mm, pgd, new);
    }
    printk("%s: 2\n", __func__);

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
        panic("pmd valid!");
    }

    if (!vmf->pte) {
        if (vma_is_anonymous(vmf->vma)) {
            return do_anonymous_page(vmf);
        } else {
            panic("not anonymous!");
        }
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
        /*
        .flags = flags,
        .pgoff = linear_page_index(vma, address),
        .gfp_mask = __get_fault_gfp_mask(vma),
        */
    };

    pgd = pgd_offset(mm, address);

    printk("%s: 1\n", __func__);
    vmf.pmd = pmd_alloc(mm, pgd, address);
    if (!vmf.pmd)
        return VM_FAULT_OOM;
    printk("%s: 2\n", __func__);

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

static int
init_module(void)
{
    printk("module[pgalloc]: init begin ...\n");

    handle_mm_fault = _handle_mm_fault;

    printk("module[pgalloc]: init end!\n");

    return 0;
}
