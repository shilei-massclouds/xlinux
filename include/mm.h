/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _RISCV_MM_H_
#define _RISCV_MM_H_

#include <page.h>
#include <atomic.h>
#include <string.h>
#include <mmzone.h>
#include <ptrace.h>
#include <pgtable.h>
#include <memblock.h>
#include <mm_types.h>
#include <page-flags.h>

#define untagged_addr(addr) (addr)

/* Page flags: | ZONE | [LAST_CPUPID] | ... | FLAGS | */
#define NODES_PGOFF     (sizeof(unsigned long)*8)
#define ZONES_PGOFF     (NODES_PGOFF - ZONES_WIDTH)

#define ZONEID_SHIFT    ZONES_SHIFT
#define ZONEID_PGOFF    ZONES_PGOFF
#define ZONEID_PGSHIFT  ZONEID_PGOFF

#define ZONES_PGSHIFT   ZONES_PGOFF
#define ZONES_MASK      ((1UL << ZONES_WIDTH) - 1)
#define ZONEID_MASK     ((1UL << ZONEID_SHIFT) - 1)

#define page_address(page)  lowmem_page_address(page)

#define offset_in_page(p)   ((unsigned long)(p) & ~PAGE_MASK)

#define ALLOC_WMARK_LOW     WMARK_LOW

/*
 * vm_flags in vm_area_struct, see mm_types.h.
 * When changing, update also include/trace/events/mmflags.h
 */
#define VM_NONE         0x00000000
#define VM_GROWSDOWN    0x00000100  /* general info on the segment */
#define VM_GROWSUP      VM_NONE

#define FOLL_WRITE  0x01    /* check pte is writable */
#define FOLL_TOUCH  0x02    /* mark page accessed */
#define FOLL_GET    0x04    /* do get_page on page */
#define FOLL_FORCE  0x10    /* get_user_pages read/write w/o permission */
#define FOLL_REMOTE 0x2000  /* we are working on non-current tsk/mm */

#define FOLL_LONGTERM   0x10000 /* mapping lifetime is indefinite: see below */
#define FOLL_PIN        0x40000 /* pages must be released via unpin_user_page */

extern struct mm_struct init_mm;

struct vm_fault {
    pmd_t *pmd; /* Pointer to pmd entry matching the 'address' */
};

struct alloc_context {
    struct zonelist *zonelist;
    struct zoneref *preferred_zoneref;

    /*
     * highest_zoneidx represents highest usable zone index of
     * the allocation request. Due to the nature of the zone,
     * memory on lower zone than the highest_zoneidx will be
     * protected by lowmem_reserve[highest_zoneidx].
     *
     * highest_zoneidx is also used by reclaim/compaction to limit
     * the target zone since higher zone than this index cannot be
     * usable for this allocation request.
     */
    enum zone_type highest_zoneidx;
};

extern unsigned long max_mapnr;

static inline void set_max_mapnr(unsigned long limit)
{
    max_mapnr = limit;
}

extern atomic_long_t _totalram_pages;

static inline void totalram_pages_add(long count)
{
    atomic_long_add(count, &_totalram_pages);
}

static __always_inline void *
lowmem_page_address(const struct page *page)
{
    return page_to_virt(page);
}

extern pgd_t early_pgd[];
extern pmd_t early_pmd[];
extern pmd_t fixmap_pmd[];
extern pte_t fixmap_pt[];
extern pgd_t swapper_pg_dir[];

extern phys_addr_t dtb_early_pa;

typedef phys_addr_t (*phys_alloc_t)(phys_addr_t size, phys_addr_t align);

void
setup_fixmap_pgd(void);

void
setup_vm_final(struct memblock_region *regions,
               unsigned long regions_cnt,
               phys_alloc_t alloc);

void
clear_flash_pgd(void);

const char *
kstrdup_const(const char *s, gfp_t gfp);

char *
kstrdup(const char *s, gfp_t gfp);

void
kfree_const(const void *x);

static inline enum zone_type
page_zonenum(const struct page *page)
{
    return (page->flags >> ZONES_PGSHIFT) & ZONES_MASK;
}

static inline struct zone *
page_zone(const struct page *page)
{
    return &NODE_DATA(0)->node_zones[page_zonenum(page)];
}

/*
 * Locate the struct page for both the matching buddy in our
 * pair (buddy1) and the combined O(n+1) page they form (page).
 *
 * 1) Any buddy B1 will have an order O twin B2 which satisfies
 * the following equation:
 *     B2 = B1 ^ (1 << O)
 * For example, if the starting buddy (buddy2) is #8 its order
 * 1 buddy is #10:
 *     B2 = 8 ^ (1 << 1) = 8 ^ 2 = 10
 *
 * 2) Any buddy B will have an order O+1 parent P which
 * satisfies the following equation:
 *     P = B & ~(1 << O)
 *
 * Assumption: *_mem_map is contiguous at least up to MAX_ORDER
 */
static inline unsigned long
__find_buddy_pfn(unsigned long page_pfn, unsigned int order)
{
    return page_pfn ^ (1 << order);
}

static inline unsigned int
page_order(struct page *page)
{
    /* PageBuddy() must be checked by the caller */
    return page_private(page);
}

static inline int page_zone_id(struct page *page)
{
    return (page->flags >> ZONEID_PGSHIFT) & ZONEID_MASK;
}

static inline struct page *virt_to_head_page(const void *x)
{
    struct page *page = virt_to_page(x);

    return compound_head(page);
}

static inline void
vma_init(struct vm_area_struct *vma, struct mm_struct *mm)
{
    static const struct vm_operations_struct dummy_vm_ops = {};

    memset(vma, 0, sizeof(*vma));
    vma->vm_mm = mm;
    vma->vm_ops = &dummy_vm_ops;
    //INIT_LIST_HEAD(&vma->anon_vma_chain);
}

static inline void vma_set_anonymous(struct vm_area_struct *vma)
{
    vma->vm_ops = NULL;
}

int insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vma);

static inline bool vma_is_anonymous(struct vm_area_struct *vma)
{
    return !vma->vm_ops;
}

void __vma_link_list(struct mm_struct *mm, struct vm_area_struct *vma,
                     struct vm_area_struct *prev);

extern unsigned long stack_guard_gap;

static inline unsigned long vm_start_gap(struct vm_area_struct *vma)
{
    unsigned long vm_start = vma->vm_start;

    if (vma->vm_flags & VM_GROWSDOWN) {
        vm_start -= stack_guard_gap;
        if (vm_start > vma->vm_start)
            vm_start = 0;
    }
    return vm_start;
}

static inline unsigned long vm_end_gap(struct vm_area_struct *vma)
{
    unsigned long vm_end = vma->vm_end;

    if (vma->vm_flags & VM_GROWSUP) {
        vm_end += stack_guard_gap;
        if (vm_end < vma->vm_end)
            vm_end = -PAGE_SIZE;
    }
    return vm_end;
}

long get_user_pages_remote(struct mm_struct *mm,
                           unsigned long start, unsigned long nr_pages,
                           unsigned int gup_flags, struct page **pages,
                           struct vm_area_struct **vmas, int *locked);

static inline unsigned long vma_pages(struct vm_area_struct *vma)
{
    return (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
}

struct vm_area_struct *
find_extend_vma(struct mm_struct *mm, unsigned long addr);

int __pmd_alloc(struct mm_struct *mm, pgd_t *pgd, unsigned long address);

static inline pmd_t *
pmd_alloc(struct mm_struct *mm, pgd_t *pgd, unsigned long address)
{
    return (unlikely(pgd_none(*pgd)) && __pmd_alloc(mm, pgd, address)) ?
        NULL: pmd_offset(pgd, address);
}

vm_fault_t
handle_mm_fault(struct vm_area_struct *vma, unsigned long address,
                unsigned int flags, struct pt_regs *regs);

#endif /* _RISCV_MM_H_ */
