// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <types.h>
#include <export.h>
#include <printk.h>
#include <xarray.h>
#include <pagemap.h>
#include <mm_types.h>

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
_add_to_page_cache_lru(struct page *page,
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

static struct page *wait_on_page_read(struct page *page)
{
    if (!IS_ERR(page)) {
        if (!PageUptodate(page))
            panic("page NOT uptodate!");
    }
    return page;
}

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

        if (err < 0)
            panic("read page error!");

        page = wait_on_page_read(page);
        if (IS_ERR(page))
            panic("wait on page error!");

        return page;
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

ssize_t generic_file_buffered_read(struct kiocb *iocb,
                                   struct iov_iter *iter,
                                   ssize_t written)
{
    pgoff_t index;
    pgoff_t last_index;
    pgoff_t prev_index;
    unsigned int prev_offset;
    unsigned long offset;      /* offset into pagecache page */
    int error = 0;
    struct file *filp = iocb->ki_filp;
    struct file_ra_state *ra = &filp->f_ra;
    struct address_space *mapping = filp->f_mapping;
    struct inode *inode = mapping->host;
    loff_t *ppos = &iocb->ki_pos;

    index = *ppos >> PAGE_SHIFT;
    prev_index = ra->prev_pos >> PAGE_SHIFT;
    prev_offset = ra->prev_pos & (PAGE_SIZE-1);
    last_index = (*ppos + iter->count + PAGE_SIZE-1) >> PAGE_SHIFT;
    offset = *ppos & ~PAGE_MASK;

    for (;;) {
        loff_t isize;
        unsigned long nr;
        unsigned long ret;
        struct page *page;
        pgoff_t end_index;

        page = find_get_page(mapping, index);
        if (!page) {
            page_cache_sync_readahead(mapping, ra, filp,
                                      index, last_index - index);
            page = find_get_page(mapping, index);
            if (unlikely(page == NULL))
                panic("no cached page!");
        }

        if (PageReadahead(page))
            panic("page readahead!");
        if (!PageUptodate(page))
            panic("page is NOT uptodate!");

        /*
         * i_size must be checked after we know the page is Uptodate.
         *
         * Checking i_size after the check allows us to calculate
         * the correct value for "nr", which means the zero-filled
         * part of the page is not copied back to userspace (unless
         * another truncate extends the file - this is desired though).
         */
        isize = i_size_read(inode);
        end_index = (isize - 1) >> PAGE_SHIFT;
        if (unlikely(!isize || index > end_index))
            goto out;

        /* nr is the maximum number of bytes to copy from this page */
        nr = PAGE_SIZE;
        if (index == end_index) {
            nr = ((isize - 1) & ~PAGE_MASK) + 1;
            if (nr <= offset) {
                goto out;
            }
        }
        nr = nr - offset;

        prev_index = index;

        /*
         * Ok, we have the page, and it's up-to-date, so
         * now we can copy it to user space...
         */
        ret = copy_page_to_iter(page, offset, nr, iter);
        offset += ret;
        index += offset >> PAGE_SHIFT;
        offset &= ~PAGE_MASK;
        prev_offset = offset;

        written += ret;
        if (!iov_iter_count(iter))
            goto out;

        panic("copy page error!");
    }

out:
    ra->prev_pos = prev_index;
    ra->prev_pos <<= PAGE_SHIFT;
    ra->prev_pos |= prev_offset;

    *ppos = ((loff_t)index << PAGE_SHIFT) + offset;
    return written ? written : error;
}
EXPORT_SYMBOL(generic_file_buffered_read);

ssize_t
generic_file_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
    ssize_t retval = 0;
    size_t count = iov_iter_count(iter);

    if (!count)
        return 0;

    return generic_file_buffered_read(iocb, iter, retval);
}
EXPORT_SYMBOL(generic_file_read_iter);

vm_fault_t filemap_fault(struct vm_fault *vmf)
{
    struct page *page;
    pgoff_t offset = vmf->pgoff;
    struct file *file = vmf->vma->vm_file;
    struct address_space *mapping = file->f_mapping;

    page = find_get_page(mapping, offset);

    panic("%s: !", __func__);
}
EXPORT_SYMBOL(filemap_fault);

const struct vm_operations_struct generic_file_vm_ops = {
    .fault          = filemap_fault,
    /*
    .map_pages      = filemap_map_pages,
    .page_mkwrite   = filemap_page_mkwrite,
    */
};

int generic_file_mmap(struct file * file, struct vm_area_struct * vma)
{
    struct address_space *mapping = file->f_mapping;

    if (!mapping->a_ops->readpage)
        panic("a_ops has no readpage!");

    vma->vm_ops = &generic_file_vm_ops;
    return 0;
}
EXPORT_SYMBOL(generic_file_mmap);

static int
init_module(void)
{
    printk("module[filemap]: init begin ...\n");

    add_to_page_cache_lru = _add_to_page_cache_lru;

    printk("module[filemap]: init end!\n");

    return 0;
}
