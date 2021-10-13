// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
#include <bug.h>
#include <page.h>
#include <sizes.h>
#include <export.h>
#include <kernel.h>
#include <string.h>
#include <mmzone.h>
#include <printk.h>
#include <memblock.h>
#include <page-flags.h>

extern void (*reserve_bootmem_region_fn)(phys_addr_t, phys_addr_t);
extern struct pglist_data contig_page_data;
extern struct page *mem_map;

unsigned long max_low_pfn;

static unsigned long
arch_zone_lowest_possible_pfn[MAX_NR_ZONES];

static unsigned long
arch_zone_highest_possible_pfn[MAX_NR_ZONES];

static char * const zone_names[MAX_NR_ZONES] = {
     "DMA32",
     "Normal",
     "Movable",
};

/**
 * find_min_pfn_with_active_regions - Find the minimum PFN registered
 *
 * Return: the minimum PFN based on information provided via
 * memblock_set_node().
 */
unsigned long
find_min_pfn_with_active_regions(void)
{
    return PHYS_PFN(memblock_start_of_DRAM());
}

void
get_pfn_range_for_nid(unsigned long *start_pfn, unsigned long *end_pfn)
{
    int i;
    unsigned long this_start_pfn;
    unsigned long this_end_pfn;

    *start_pfn = -1UL;
    *end_pfn = 0;

    for_each_mem_pfn_range(i, &this_start_pfn, &this_end_pfn) {
        *start_pfn = min(*start_pfn, this_start_pfn);
        *end_pfn = max(*end_pfn, this_end_pfn);
    }

    if (*start_pfn == -1UL)
        *start_pfn = 0;
}

static unsigned long
zone_spanned_pages_in_node(int nid,
                           unsigned long zone_type,
                           unsigned long node_start_pfn,
                           unsigned long node_end_pfn,
                           unsigned long *zone_start_pfn,
                           unsigned long *zone_end_pfn)
{
    unsigned long zone_low = arch_zone_lowest_possible_pfn[zone_type];
    unsigned long zone_high = arch_zone_highest_possible_pfn[zone_type];
    /* When hotadd a new node from cpu_up(), the node should be empty */
    if (!node_start_pfn && !node_end_pfn)
        return 0;

    /* Get the start and end of the zone */
    *zone_start_pfn = clamp(node_start_pfn, zone_low, zone_high);
    *zone_end_pfn = clamp(node_end_pfn, zone_low, zone_high);

    /* Check that this node has pages within the zone's required range */
    if (*zone_end_pfn < node_start_pfn || *zone_start_pfn > node_end_pfn)
        return 0;

    /* Move the zone boundaries inside the node if necessary */
    *zone_end_pfn = min(*zone_end_pfn, node_end_pfn);
    *zone_start_pfn = max(*zone_start_pfn, node_start_pfn);

    /* Return the spanned pages */
    return *zone_end_pfn - *zone_start_pfn;
}

/*
 * Return the number of holes in a range on a node. If nid is MAX_NUMNODES,
 * then all holes in the requested range will be accounted for.
 */
unsigned long
__absent_pages_in_range(int nid,
                        unsigned long range_start_pfn,
                        unsigned long range_end_pfn)
{
    unsigned long nr_absent = range_end_pfn - range_start_pfn;
    unsigned long start_pfn, end_pfn;
    int i;

    for_each_mem_pfn_range(i, &start_pfn, &end_pfn) {
        start_pfn = clamp(start_pfn, range_start_pfn, range_end_pfn);
        end_pfn = clamp(end_pfn, range_start_pfn, range_end_pfn);
        nr_absent -= end_pfn - start_pfn;
    }

    return nr_absent;
}

/* Return the number of page frames in holes in a zone on a node */
static unsigned long
zone_absent_pages_in_node(int nid,
                          unsigned long zone_type,
                          unsigned long node_start_pfn,
                          unsigned long node_end_pfn)
{
    unsigned long zone_low = arch_zone_lowest_possible_pfn[zone_type];
    unsigned long zone_high = arch_zone_highest_possible_pfn[zone_type];
    unsigned long zone_start_pfn, zone_end_pfn;

    /* When hotadd a new node from cpu_up(), the node should be empty */
    if (!node_start_pfn && !node_end_pfn)
        return 0;

    zone_start_pfn = clamp(node_start_pfn, zone_low, zone_high);
    zone_end_pfn = clamp(node_end_pfn, zone_low, zone_high);

    return __absent_pages_in_range(nid, zone_start_pfn, zone_end_pfn);
}

static void
calculate_node_totalpages(struct pglist_data *pgdat,
                          unsigned long node_start_pfn,
                          unsigned long node_end_pfn)
{
    enum zone_type i;
    unsigned long totalpages = 0;
    unsigned long realtotalpages = 0;

    for (i = 0; i < MAX_NR_ZONES; i++) {
        struct zone *zone = pgdat->node_zones + i;
        unsigned long zone_start_pfn, zone_end_pfn;
        unsigned long spanned, absent;
        unsigned long size, real_size;

        spanned = zone_spanned_pages_in_node(0, i,
                                             node_start_pfn, node_end_pfn,
                                             &zone_start_pfn, &zone_end_pfn);

        absent = zone_absent_pages_in_node(0, i, node_start_pfn, node_end_pfn);

        size = spanned;
        real_size = size - absent;

        if (size)
            zone->zone_start_pfn = zone_start_pfn;
        else
            zone->zone_start_pfn = 0;

        zone->spanned_pages = size;
        zone->present_pages = real_size;

        totalpages += size;
        realtotalpages += real_size;
    }

    pgdat->node_spanned_pages = totalpages;
    pgdat->node_present_pages = realtotalpages;
    printk("On node 0 totalpages: %lu\n", realtotalpages);
}

static void
alloc_node_mem_map(struct pglist_data *pgdat)
{
    unsigned long start = 0;
    unsigned long offset = 0;

    /* Skip empty nodes */
    if (!pgdat->node_spanned_pages)
        return;

    start = pgdat->node_start_pfn & ~(MAX_ORDER_NR_PAGES - 1);
    offset = pgdat->node_start_pfn - start;

    if (!pgdat->node_mem_map) {
        unsigned long size, end;
        struct page *map;

        /*
         * The zone's endpoints aren't required to be MAX_ORDER
         * aligned but the node_mem_map endpoints must be in order
         * for the buddy allocator to function correctly.
         */
        end = pgdat_end_pfn(pgdat);
        end = ALIGN(end, MAX_ORDER_NR_PAGES);
        size = (end - start) * sizeof(struct page);
        map = memblock_alloc_node(size, SMP_CACHE_BYTES);
        if (!map)
            panic("Failed to allocate %ld bytes for node 0 memory map\n",
                  size);
        pgdat->node_mem_map = map + offset;
    }

    /*
     * With no DISCONTIG, the global mem_map is just set as node 0's
     */
    if (pgdat == NODE_DATA(0)) {
        mem_map = NODE_DATA(0)->node_mem_map;
        if (page_to_pfn(mem_map) != pgdat->node_start_pfn)
            mem_map -= offset;
    }

    printk("%s: node 0, pgdat %lx, node_mem_map %lx\n",
           __func__, (unsigned long)pgdat,
           (unsigned long)pgdat->node_mem_map);
}

static void
zone_init_free_lists(struct zone *zone)
{
    unsigned int order, t;
    for_each_migratetype_order(order, t) {
        INIT_LIST_HEAD(&zone->free_area[order].free_list[t]);
        zone->free_area[order].nr_free = 0;
    }
}

void
init_currently_empty_zone(struct zone *zone,
                          unsigned long zone_start_pfn,
                          unsigned long size)
{
    struct pglist_data *pgdat = zone->zone_pgdat;
    int zone_idx = zone_idx(zone) + 1;

    if (zone_idx > pgdat->nr_zones)
        pgdat->nr_zones = zone_idx;

    zone->zone_start_pfn = zone_start_pfn;

    printk("Initialising map zone %lu pfns %lu -> %lu\n",
           (unsigned long)zone_idx(zone),
           zone_start_pfn, (zone_start_pfn + size));

    zone_init_free_lists(zone);
    zone->initialized = 1;
}

static void
zone_init_internals(struct zone *zone,
                    enum zone_type idx,
                    unsigned long remaining_pages)
{
    atomic_long_set(&zone->managed_pages, remaining_pages);
    zone->name = zone_names[idx];
    zone->zone_pgdat = NODE_DATA(0);
}

static unsigned long
calc_memmap_size(unsigned long spanned_pages,
                 unsigned long present_pages)
{
    return PAGE_ALIGN(spanned_pages * sizeof(struct page)) >> PAGE_SHIFT;
}

static void
free_area_init_core(struct pglist_data *pgdat)
{
    enum zone_type j;

    for (j = 0; j < MAX_NR_ZONES; j++) {
        unsigned long size;
        unsigned long freesize;
        unsigned long memmap_pages;
        struct zone *zone = pgdat->node_zones + j;
        unsigned long zone_start_pfn = zone->zone_start_pfn;

        size = zone->spanned_pages;
        freesize = zone->present_pages;

        memmap_pages = calc_memmap_size(size, freesize);
        if (freesize >= memmap_pages) {
            freesize -= memmap_pages;
            if (memmap_pages)
                printk("  %s zone: %lu pages used for memmap\n",
                       zone_names[j], memmap_pages);
        } else {
            printk("  %s zone: %lu pages exceeds freesize %lu\n",
                   zone_names[j], memmap_pages, freesize);
        }

        /*
         * Set an approximate value for lowmem here, it will be adjusted
         * when the bootmem allocator frees pages into the buddy system.
         * And all highmem pages will be managed by the buddy system.
         */
        zone_init_internals(zone, j, freesize);

        if (!size)
            continue;

        init_currently_empty_zone(zone, zone_start_pfn, size);
    }
}

static void
free_area_init_node(void)
{
    unsigned long start_pfn = 0;
    unsigned long end_pfn = 0;
    pg_data_t *pgdat = NODE_DATA(0);

    /* pg_data_t should be reset to zero when it's allocated */
    BUG_ON(pgdat->nr_zones || pgdat->kswapd_highest_zoneidx);

    get_pfn_range_for_nid(&start_pfn, &end_pfn);

    pgdat->node_start_pfn = start_pfn;

    printk("Initmem setup [mem %lx-%lx]\n",
           (u64)start_pfn << PAGE_SHIFT,
           end_pfn ? ((u64)end_pfn << PAGE_SHIFT) - 1 : 0);
    calculate_node_totalpages(pgdat, start_pfn, end_pfn);

    alloc_node_mem_map(pgdat);
    free_area_init_core(pgdat);
}

void
free_area_init(unsigned long *max_zone_pfn)
{
    int i;
    unsigned long start_pfn;
    unsigned long end_pfn;
    pg_data_t *pgdat = NODE_DATA(0);

    memset(arch_zone_lowest_possible_pfn, 0,
           sizeof(arch_zone_lowest_possible_pfn));
    memset(arch_zone_highest_possible_pfn, 0,
           sizeof(arch_zone_highest_possible_pfn));

    start_pfn = find_min_pfn_with_active_regions();

    for (i = 0; i < MAX_NR_ZONES; i++) {
        if (i == ZONE_MOVABLE)
            continue;

        end_pfn = max(max_zone_pfn[i], start_pfn);
        arch_zone_lowest_possible_pfn[i] = start_pfn;
        arch_zone_highest_possible_pfn[i] = end_pfn;

        start_pfn = end_pfn;
    }

    /* Print out the zone ranges */
    printk("Zone ranges:\n");
    for (i = 0; i < MAX_NR_ZONES; i++) {
        if (i == ZONE_MOVABLE)
            continue;

        if (arch_zone_lowest_possible_pfn[i] ==
            arch_zone_highest_possible_pfn[i])
            printk("%s empty\n", zone_names[i]);
        else
            printk("%s [mem %lx-%lx)\n",
                   zone_names[i],
                   (u64)arch_zone_lowest_possible_pfn[i] << PAGE_SHIFT,
                   ((u64)arch_zone_highest_possible_pfn[i] << PAGE_SHIFT) - 1);
    }

    free_area_init_node();
}

static void
zone_sizes_init(void)
{
    unsigned long max_zone_pfns[MAX_NR_ZONES] = {0, };

    max_zone_pfns[ZONE_DMA32] =
        PFN_DOWN(min(4UL * SZ_1G, (unsigned long) PFN_PHYS(max_low_pfn)));
    max_zone_pfns[ZONE_NORMAL] = max_low_pfn;

    free_area_init(max_zone_pfns);
}

void
reserve_bootmem_region(phys_addr_t start, phys_addr_t end)
{
    unsigned long start_pfn = PFN_DOWN(start);
    unsigned long end_pfn = PFN_UP(end);

    for (; start_pfn < end_pfn; start_pfn++) {
        if (pfn_valid(start_pfn)) {
            struct page *page = pfn_to_page(start_pfn);

            /* Avoid false-positive PageTail() */
            INIT_LIST_HEAD(&page->lru);

            /*
             * no need for atomic set_bit because the struct
             * page is not visible yet so nobody should
             * access it yet.
             */
            __SetPageReserved(page);
        }
    }
}

static int
init_module(void)
{
    printk("module[buddy]: init begin ...\n");

    reserve_bootmem_region_fn = reserve_bootmem_region;

    max_low_pfn = PFN_DOWN(memblock_end_of_DRAM());
    set_max_mapnr(max_low_pfn);

    zone_sizes_init();
    memblock_free_all();

    printk("module[buddy]: init end!\n");
    return 0;
}
