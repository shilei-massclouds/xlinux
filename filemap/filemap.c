// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <types.h>
#include <export.h>
#include <printk.h>
#include <xarray.h>
#include <pagemap.h>

static int
__add_to_page_cache_locked(struct page *page,
                           struct address_space *mapping,
                           pgoff_t offset, gfp_t gfp_mask)
{
    void *old;
    XA_STATE(xas, &mapping->i_pages, offset);

    page->mapping = mapping;
    page->index = offset;

    old = xas_load(&xas);
    if (old && !xa_is_value(old))
        panic("already exist! (%p)", old);

    xas_store(&xas, page);
    if (xas_error(&xas))
        panic("can not store!");

    mapping->nrpages++;
    return 0;
}

int
add_to_page_cache_lru(struct page *page,
                      struct address_space *mapping,
                      pgoff_t offset,
                      gfp_t gfp_mask)
{
    int ret;

    ret = __add_to_page_cache_locked(page, mapping, offset, gfp_mask);
    BUG_ON(ret);
    BUG_ON(PageActive(page));
    //lru_cache_add(page);

    return ret;
}
EXPORT_SYMBOL(add_to_page_cache_lru);

struct page *
find_get_entry(struct address_space *mapping, pgoff_t offset)
{
    struct page *page;
    XA_STATE(xas, &mapping->i_pages, offset);

    page = xas_load(&xas);
    return page;
}

struct page *
pagecache_get_page(struct address_space *mapping,
                   pgoff_t index,
                   gfp_t gfp_mask)
{
    int err;
    struct page *page;

    page = find_get_entry(mapping, index);
    if (page)
        return page;

    page = __page_cache_alloc(gfp_mask);
    if (!page)
        return NULL;

    err = add_to_page_cache_lru(page, mapping, index, gfp_mask);
    if (unlikely(err))
        panic("add page to cache lru error!");

    return page;
}
EXPORT_SYMBOL(pagecache_get_page);

static int
init_module(void)
{
    printk("module[filemap]: init begin ...\n");
    printk("module[filemap]: init end!\n");

    return 0;
}
