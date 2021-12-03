/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PAGEMAP_H
#define _LINUX_PAGEMAP_H

#include <fs.h>
#include <gfp.h>
#include <mm_types.h>
#include <page-flags.h>

#define FGP_ACCESSED        0x00000001

struct page *
pagecache_get_page(struct address_space *mapping,
                   pgoff_t offset,
                   gfp_t cache_gfp_mask);

static inline struct page *
find_or_create_page(struct address_space *mapping,
                    pgoff_t index,
                    gfp_t gfp_mask)
{
    return pagecache_get_page(mapping, index, gfp_mask);
}

static inline struct page *
__page_cache_alloc(gfp_t gfp)
{
    return alloc_pages(gfp, 0);
}

static inline void attach_page_private(struct page *page, void *data)
{
    set_page_private(page, (unsigned long)data);
    SetPagePrivate(page);
}

static inline struct page *
find_get_page_flags(struct address_space *mapping, pgoff_t offset)
{
    return pagecache_get_page(mapping, offset, 0);
}

#endif /* _LINUX_PAGEMAP_H */
