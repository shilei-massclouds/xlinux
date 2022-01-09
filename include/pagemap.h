/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PAGEMAP_H
#define _LINUX_PAGEMAP_H

#include <fs.h>
#include <gfp.h>
#include <filemap.h>
#include <mm_types.h>
#include <page-flags.h>

#define FGP_ACCESSED    0x00000001
#define FGP_LOCK        0x00000002
#define FGP_CREAT       0x00000004

struct page *
pagecache_get_page(struct address_space *mapping, pgoff_t offset,
                   int fgp_flags, gfp_t cache_gfp_mask);

static inline struct page *
find_or_create_page(struct address_space *mapping,
                    pgoff_t index,
                    gfp_t gfp_mask)
{
    return pagecache_get_page(mapping, index,
                              FGP_LOCK|FGP_ACCESSED|FGP_CREAT,
                              gfp_mask);
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
find_get_page_flags(struct address_space *mapping, pgoff_t offset, int fgp_flags)
{
    return pagecache_get_page(mapping, offset, fgp_flags, 0);
}

static inline unsigned long dir_pages(struct inode *inode)
{
    return (unsigned long)(inode->i_size + PAGE_SIZE - 1) >> PAGE_SHIFT;
}

static inline struct page *
read_mapping_page(struct address_space *mapping,
                  pgoff_t index, void *data)
{
    return read_cache_page(mapping, index, NULL, data);
}

static inline gfp_t mapping_gfp_mask(struct address_space *mapping)
{
    return mapping->gfp_mask;
}

/**
 * find_get_page - find and get a page reference
 * @mapping: the address_space to search
 * @offset: the page index
 *
 * Looks up the page cache slot at @mapping & @offset.  If there is a
 * page cache page, it is returned with an increased refcount.
 *
 * Otherwise, %NULL is returned.
 */
static inline struct page *
find_get_page(struct address_space *mapping, pgoff_t offset)
{
    return pagecache_get_page(mapping, offset, 0, 0);
}

static inline gfp_t
mapping_gfp_constraint(struct address_space *mapping, gfp_t gfp_mask)
{
    return mapping_gfp_mask(mapping) & gfp_mask;
}

#endif /* _LINUX_PAGEMAP_H */
