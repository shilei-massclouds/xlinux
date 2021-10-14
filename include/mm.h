/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _RISCV_MM_H_
#define _RISCV_MM_H_

#include <page.h>
#include <atomic.h>
#include <mmzone.h>
#include <memblock.h>

/* Page flags: | ZONE | [LAST_CPUPID] | ... | FLAGS | */
#define NODES_PGOFF     (sizeof(unsigned long)*8)
#define ZONES_PGOFF     (NODES_PGOFF - ZONES_WIDTH)

#define ZONEID_SHIFT    ZONES_SHIFT
#define ZONEID_PGOFF    ZONES_PGOFF
#define ZONEID_PGSHIFT  ZONEID_PGOFF

#define ZONES_PGSHIFT   ZONES_PGOFF
#define ZONES_MASK      ((1UL << ZONES_WIDTH) - 1)
#define ZONEID_MASK     ((1UL << ZONEID_SHIFT) - 1)

#define page_address(page) lowmem_page_address(page)

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
    return memblock_alloc(PAGE_SIZE, PAGE_SIZE);
    //Todo: replace up with below
    //return page_to_virt(page);
}

extern pge_t early_pgd[];
extern pme_t early_pmd[];
extern pme_t fixmap_pmd[];
extern pte_t fixmap_pt[];
extern pge_t swapper_pgd[];

extern phys_addr_t dtb_early_pa;

typedef phys_addr_t (*phys_alloc_t)(phys_addr_t size, phys_addr_t align);

void
setup_fixmap_pge(void);

void
setup_vm_final(struct memblock_region *regions,
               unsigned long regions_cnt,
               phys_alloc_t alloc);

void
clear_flash_pge(void);

const char *
kstrdup_const(const char *s);

char *
kstrdup(const char *s);

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

static inline void
set_page_private(struct page *page, unsigned long private)
{
    page->private = private;
}

static inline int page_zone_id(struct page *page)
{
    return (page->flags >> ZONEID_PGSHIFT) & ZONEID_MASK;
}

#endif /* _RISCV_MM_H_ */
