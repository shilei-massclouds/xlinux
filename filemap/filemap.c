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
    if (old)
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

    printk("%s: page(%lx)\n", __func__, page);
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
pagecache_get_page(struct address_space *mapping, pgoff_t index,
                   int fgp_flags, gfp_t gfp_mask)
{
    int err;
    struct page *page;

    page = find_get_entry(mapping, index);
    if (page)
        return page;

    if (!page && (fgp_flags & FGP_CREAT)) {
        page = __page_cache_alloc(gfp_mask);
        if (!page)
            return NULL;

        err = add_to_page_cache_lru(page, mapping, index, gfp_mask);
        if (unlikely(err))
            panic("add page to cache lru error!");
    }

    return page;
}
EXPORT_SYMBOL(pagecache_get_page);

static struct page *
do_read_cache_page(struct address_space *mapping,
                   pgoff_t index,
                   int (*filler)(void *, struct page *),
                   void *data,
                   gfp_t gfp)
{
    int err;
    struct page *page;

    printk("%s: 1 index(%lu)\n", __func__, index);
    page = find_get_page(mapping, index);
    printk("%s: page(%lx)\n", __func__, page);
    if (!page) {
        page = __page_cache_alloc(gfp);
        if (!page)
            panic("out of memory!");
        err = add_to_page_cache_lru(page, mapping, index, gfp);
        if (unlikely(err))
            panic("add to page cache error!");

        if (filler)
            err = filler(data, page);
        else
            err = mapping->a_ops->readpage(data, page);

        panic("%s: 1", __func__);
    }

    if (PageUptodate(page))
        return page;

    panic("%s: !", __func__);
}

struct page *
read_cache_page(struct address_space *mapping,
                pgoff_t index,
                int (*filler)(void *, struct page *),
                void *data)
{
    return do_read_cache_page(mapping, index, filler, data,
                              mapping_gfp_mask(mapping));
}
EXPORT_SYMBOL(read_cache_page);

void page_endio(struct page *page, bool is_write, int err)
{
    if (!is_write) {
        if (!err) {
            SetPageUptodate(page);
        } else {
            ClearPageUptodate(page);
            SetPageError(page);
        }
        //unlock_page(page);
    } else {
        if (err) {
            panic("err: %d", err);
        }
        //end_page_writeback(page);
    }
}
EXPORT_SYMBOL(page_endio);

static int
init_module(void)
{
    printk("module[filemap]: init begin ...\n");
    printk("module[filemap]: init end!\n");

    return 0;
}
