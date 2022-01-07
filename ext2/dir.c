// SPDX-License-Identifier: GPL-2.0

#include <errno.h>
#include <dcache.h>
#include <fs/ext2.h>
#include <pagemap.h>
#include <highmem.h>

static struct page *
ext2_get_page(struct inode *dir, unsigned long n, int quiet)
{
    struct address_space *mapping = dir->i_mapping;
    struct page *page = read_mapping_page(mapping, n, NULL);
    if (!IS_ERR(page)) {
        kmap(page);
    }
    return page;
}

/*
 *  ext2_find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the page in which the entry was found (as a parameter - res_page),
 * and the entry itself. Page is returned mapped and unlocked.
 * Entry is guaranteed to be valid.
 */
struct ext2_dir_entry_2 *
ext2_find_entry(struct inode *dir, const struct qstr *child,
                struct page **res_page)
{
    unsigned long start, n;
    struct page *page = NULL;
    unsigned long npages = dir_pages(dir);
    struct ext2_inode_info *ei = EXT2_I(dir);

    if (npages == 0)
        return ERR_PTR(-ENOENT);

    /* OFFSET_CACHE */
    *res_page = NULL;

    start = ei->i_dir_start_lookup;
    if (start >= npages)
        start = 0;
    n = start;
    do {
        char *kaddr;
        page = ext2_get_page(dir, n, 0);
        if (IS_ERR(page))
            panic("bad page!");

        panic("%s: 1", __func__);
    } while(n != start);

    panic("%s: !", __func__);
}

int ext2_inode_by_name(struct inode *dir, const struct qstr *child,
                       ino_t *ino)
{
    struct page *page;
    struct ext2_dir_entry_2 *de;

    de = ext2_find_entry(dir, child, &page);
    /*
    if (IS_ERR(de))
        return PTR_ERR(de);

    *ino = le32_to_cpu(de->inode);
    ext2_put_page(page);
    return 0;
    */
    panic("%s: [%s]!", __func__, child->name);
}
