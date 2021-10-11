/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MMZONE_H
#define _LINUX_MMZONE_H

#define MAX_ORDER 11
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))

#define MAX_NR_ZONES 3  /* __MAX_NR_ZONES */

enum zone_type {
    /*
     * ZONE_DMA and ZONE_DMA32 are used when there are peripherals not able
     * to DMA to all of the addressable memory (ZONE_NORMAL).
     * On architectures where this area covers the whole 32 bit address
     * space ZONE_DMA32 is used. ZONE_DMA is left for the ones with smaller
     * DMA addressing constraints. This distinction is important as a 32bit
     * DMA mask is assumed when ZONE_DMA32 is defined. Some 64-bit
     * platforms may need both zones as they support peripherals with
     * different DMA addressing limitations.
     *
     * ia64 and riscv only use ZONE_DMA32.
     */
    ZONE_DMA32,

    /*
     * Normal addressable memory is in ZONE_NORMAL. DMA operations can be
     * performed on pages in ZONE_NORMAL if the DMA devices support
     * transfers to all addressable memory.
     */
    ZONE_NORMAL,

    ZONE_MOVABLE,

    __MAX_NR_ZONES
};

struct zone {
    /* zone_start_pfn == zone_start_paddr >> PAGE_SHIFT */
    unsigned long   zone_start_pfn;

    unsigned long   spanned_pages;
    unsigned long   present_pages;
};

typedef struct pglist_data {
    /*
     * node_zones contains just the zones for THIS node. Not all of the
     * zones may be populated, but it is the full list. It is referenced by
     * this node's node_zonelists as well as other node's node_zonelists.
     */
    struct zone node_zones[MAX_NR_ZONES];

    int nr_zones; /* number of populated zones in this node */
    enum zone_type kswapd_highest_zoneidx;

    unsigned long node_start_pfn;
    unsigned long node_present_pages; /* total number of physical pages */
    unsigned long node_spanned_pages; /* total size of physical page range,
                                         including holes */

    struct page *node_mem_map;
} pg_data_t;

extern struct pglist_data contig_page_data;
#define NODE_DATA(nid)  (&contig_page_data)

static inline unsigned long pgdat_end_pfn(pg_data_t *pgdat)
{
    return pgdat->node_start_pfn + pgdat->node_spanned_pages;
}

#endif /* _LINUX_MMZONE_H */
