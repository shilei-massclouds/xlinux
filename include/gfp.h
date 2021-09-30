/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_GFP_H
#define __LINUX_GFP_H

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

#endif /* __LINUX_GFP_H */
