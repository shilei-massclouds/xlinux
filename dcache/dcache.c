/* SPDX-License-Identifier: GPL-2.0-only */

#include <slab.h>
#include <dcache.h>
#include <export.h>
#include <printk.h>

static struct kmem_cache *dentry_cache;

static struct dentry *
__d_alloc(struct super_block *sb, const struct qstr *name)
{
    struct dentry *dentry;

    dentry = kmem_cache_alloc(dentry_cache, GFP_KERNEL);
    if (!dentry)
        return NULL;

    dentry->d_parent = dentry;
    dentry->d_sb = sb;

    return dentry;
}

struct dentry *
d_alloc_anon(struct super_block *sb)
{
    return __d_alloc(sb, NULL);
}
EXPORT_SYMBOL(d_alloc_anon);

static void
__d_instantiate(struct dentry *dentry, struct inode *inode)
{
    /* Todo */
}

void
d_instantiate(struct dentry *entry, struct inode * inode)
{
    if (inode) {
        __d_instantiate(entry, inode);
    }
}
EXPORT_SYMBOL(d_instantiate);

struct dentry *
d_make_root(struct inode *root_inode)
{
    struct dentry *res = NULL;

    if (root_inode) {
        res = d_alloc_anon(root_inode->i_sb);
        if (res)
            d_instantiate(res, root_inode);
        else
            iput(root_inode);
    }
    return res;
}
EXPORT_SYMBOL(d_make_root);

static void
dcache_init(void)
{
    dentry_cache = KMEM_CACHE_USERCOPY(dentry,
                                       SLAB_RECLAIM_ACCOUNT|SLAB_PANIC|
                                       SLAB_MEM_SPREAD|SLAB_ACCOUNT,
                                       d_iname);
}

static int
init_module(void)
{
    printk("module[dcache]: init begin ...\n");
    dcache_init();
    printk("module[dcache]: init end!\n");
    return 0;
}
