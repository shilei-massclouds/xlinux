/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_GFP_H
#define __LINUX_GFP_H

#include <bug.h>
#include <mmzone.h>
#include <kernel.h>

#define ___GFP_DMA              0x01u
#define ___GFP_HIGHMEM          0x02u
#define ___GFP_DMA32            0x04u
#define ___GFP_MOVABLE          0x08u
#define ___GFP_RECLAIMABLE      0x10u
#define ___GFP_IO               0x40u
#define ___GFP_FS               0x80u
#define ___GFP_ZERO             0x100u
#define ___GFP_DIRECT_RECLAIM   0x400u
#define ___GFP_KSWAPD_RECLAIM   0x800u
#define ___GFP_NOFAIL           0x8000u

/*
 * %__GFP_DIRECT_RECLAIM indicates that the caller may enter direct
 * reclaim. This flag can be cleared to avoid unnecessary delays when
 * a fallback option is available.
 *
 * %__GFP_KSWAPD_RECLAIM indicates that the caller wants to wake
 * kswapd when the low watermark is reached and have it reclaim pages
 * until the high watermark is reached. A caller may wish to clear this
 * flag when fallback options are available and the reclaim is likely
 * to disrupt the system. The canonical example is THP allocation
 * where a fallback is cheap but reclaim/compaction may cause indirect
 * stalls.
 *
 * %__GFP_RECLAIM is shorthand to allow/forbid both direct and
 * kswapd reclaim.
 */

#define __GFP_IO    ((gfp_t)___GFP_IO)
#define __GFP_FS    ((gfp_t)___GFP_FS)
#define __GFP_ZERO  ((gfp_t)___GFP_ZERO)

#define __GFP_NOFAIL    ((gfp_t)___GFP_NOFAIL)

#define __GFP_RECLAIMABLE ((gfp_t)___GFP_RECLAIMABLE)

#define __GFP_DIRECT_RECLAIM    ((gfp_t)___GFP_DIRECT_RECLAIM) /* Caller can reclaim */
#define __GFP_KSWAPD_RECLAIM    ((gfp_t)___GFP_KSWAPD_RECLAIM) /* kswapd can wake */

#define __GFP_RECLAIM ((gfp_t)(___GFP_DIRECT_RECLAIM|___GFP_KSWAPD_RECLAIM))

/*
 * %GFP_KERNEL is typical for kernel-internal allocations.
 * The caller requires %ZONE_NORMAL or a lower zone for direct access
 * but can direct reclaim.
 */

#define GFP_KERNEL  (__GFP_RECLAIM | __GFP_IO | __GFP_FS)
#define GFP_NOWAIT  (__GFP_KSWAPD_RECLAIM)

struct page *
__alloc_pages_nodemask(gfp_t gfp_mask, unsigned int order);

static inline struct page *
__alloc_pages(gfp_t gfp_mask, unsigned int order, int preferred_nid)
{
    return __alloc_pages_nodemask(gfp_mask, order);
}

/*
 * Allocate pages, preferring the node given as nid. The node must be valid and
 * online. For more general interface, see alloc_pages_node().
 */
static inline struct page *
__alloc_pages_node(int nid, gfp_t gfp_mask, unsigned int order)
{
    BUG_ON(nid < 0 || nid >= MAX_NUMNODES);
    return __alloc_pages(gfp_mask, order, nid);
}

static inline struct page *
alloc_pages_node(int nid, gfp_t gfp_mask, unsigned int order)
{
    return __alloc_pages_node(nid, gfp_mask, order);
}

#define alloc_pages(gfp_mask, order) alloc_pages_node(0, gfp_mask, order)

#define alloc_page(gfp_mask) alloc_pages(gfp_mask, 0)

unsigned long __get_free_pages(gfp_t gfp_mask, unsigned int order);

#define __GFP_DMA       ((__force gfp_t)___GFP_DMA)
#define __GFP_HIGHMEM   ((__force gfp_t)___GFP_HIGHMEM)
#define __GFP_DMA32     ((__force gfp_t)___GFP_DMA32)
#define __GFP_MOVABLE   ((__force gfp_t)___GFP_MOVABLE)  /* ZONE_MOVABLE allowed */
#define GFP_ZONEMASK \
    (__GFP_DMA|__GFP_HIGHMEM|__GFP_DMA32|__GFP_MOVABLE)

#define GFP_ZONE_TABLE ( \
    (ZONE_NORMAL << 0 * GFP_ZONES_SHIFT) | \
    (ZONE_NORMAL << ___GFP_DMA * GFP_ZONES_SHIFT) | \
    (ZONE_NORMAL << ___GFP_HIGHMEM * GFP_ZONES_SHIFT) | \
    (ZONE_DMA32  << ___GFP_DMA32 * GFP_ZONES_SHIFT) \
)

#define GFP_ZONES_SHIFT ZONES_SHIFT

static inline enum zone_type gfp_zone(gfp_t flags)
{
    enum zone_type z;
    int bit = (__force int) (flags & GFP_ZONEMASK);

    z = (GFP_ZONE_TABLE >> (bit * GFP_ZONES_SHIFT)) &
        ((1 << GFP_ZONES_SHIFT) - 1);
    return z;
}

static inline int gfp_zonelist(gfp_t flags)
{
    return ZONELIST_FALLBACK;
}

/*
 * We get the zone list from the current node and the gfp_mask.
 * This zone list contains a maximum of MAXNODES*MAX_NR_ZONES zones.
 * There are two zonelists per node, one for all zones with memory and
 * one containing just zones from the node the zonelist belongs to.
 *
 * For the normal case of non-DISCONTIGMEM systems the NODE_DATA() gets
 * optimized to &contig_page_data at compile-time.
 */
static inline struct zonelist *
node_zonelist(gfp_t flags)
{
    return NODE_DATA(0)->node_zonelists + gfp_zonelist(flags);
}

#endif /* __LINUX_GFP_H */
