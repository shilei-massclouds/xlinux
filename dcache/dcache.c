/* SPDX-License-Identifier: GPL-2.0-only */

#include <slab.h>
#include <dcache.h>
#include <export.h>
#include <limits.h>
#include <printk.h>
#include <string.h>

static struct kmem_cache *dentry_cache;

/* SLAB cache for __getname() consumers */
struct kmem_cache *names_cachep;
EXPORT_SYMBOL(names_cachep);

const struct qstr slash_name = QSTR_INIT("/", 1);
EXPORT_SYMBOL(slash_name);

struct external_name {
    atomic_t count;
    unsigned char name[];
};

static struct dentry *
__d_alloc(struct super_block *sb, const struct qstr *name)
{
    char *dname;
    struct dentry *dentry;

    dentry = kmem_cache_alloc(dentry_cache, GFP_KERNEL);
    if (!dentry)
        return NULL;

    dentry->d_iname[DNAME_INLINE_LEN-1] = 0;
    if (unlikely(!name)) {
        name = &slash_name;
        dname = dentry->d_iname;
    } else if (name->len > DNAME_INLINE_LEN-1) {
        size_t size = offsetof(struct external_name, name[1]);
        struct external_name *p = kmalloc(size + name->len,
                                          GFP_KERNEL_ACCOUNT |
                                          __GFP_RECLAIMABLE);
        if (!p) {
            kmem_cache_free(dentry_cache, dentry);
            return NULL;
        }
        atomic_set(&p->count, 1);
        dname = p->name;
    } else {
        dname = dentry->d_iname;
    }

    dentry->d_name.len = name->len;
    dentry->d_name.hash = name->hash;
    memcpy(dname, name->name, name->len);
    dname[name->len] = 0;

    dentry->d_name.name = dname;

    dentry->d_parent = dentry;
    dentry->d_sb = sb;
    INIT_LIST_HEAD(&dentry->d_child);
    INIT_LIST_HEAD(&dentry->d_subdirs);
    return dentry;
}

struct dentry *
d_alloc_anon(struct super_block *sb)
{
    return __d_alloc(sb, NULL);
}
EXPORT_SYMBOL(d_alloc_anon);

static inline void
__d_set_inode_and_type(struct dentry *dentry,
                       struct inode *inode)
{
    dentry->d_inode = inode;
}

static void
__d_instantiate(struct dentry *dentry, struct inode *inode)
{
    __d_set_inode_and_type(dentry, inode);
}

void
d_instantiate(struct dentry *entry, struct inode *inode)
{
    if (inode)
        __d_instantiate(entry, inode);
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

struct dentry *
d_lookup(const struct dentry *parent, const struct qstr *name)
{
    /* Todo */
    return NULL;
}
EXPORT_SYMBOL(d_lookup);

struct dentry *
d_alloc(struct dentry *parent, const struct qstr *name)
{
    struct dentry *dentry = __d_alloc(parent->d_sb, name);
    if (!dentry)
        return NULL;

    dentry->d_parent = parent;
    list_add(&dentry->d_child, &parent->d_subdirs);
    return dentry;
}
EXPORT_SYMBOL(d_alloc);

static void
dcache_init(void)
{
    dentry_cache = KMEM_CACHE_USERCOPY(dentry,
                                       SLAB_RECLAIM_ACCOUNT|SLAB_PANIC|
                                       SLAB_MEM_SPREAD|SLAB_ACCOUNT,
                                       d_iname);

    names_cachep =
        kmem_cache_create_usercopy("names_cache", PATH_MAX, 0,
                                   SLAB_HWCACHE_ALIGN|SLAB_PANIC,
                                   0, PATH_MAX, NULL);
}

static int
init_module(void)
{
    printk("module[dcache]: init begin ...\n");
    dcache_init();
    printk("module[dcache]: init end!\n");
    return 0;
}
