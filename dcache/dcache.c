/* SPDX-License-Identifier: GPL-2.0-only */

#include <slab.h>
#include <dcache.h>
#include <export.h>
#include <limits.h>
#include <printk.h>
#include <string.h>
#include <hashtable.h>
#include <stringhash.h>

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

static unsigned int d_hash_shift;

static struct hlist_bl_head *dentry_hashtable;

static inline struct hlist_bl_head *d_hash(unsigned int hash)
{
    return dentry_hashtable + (hash >> d_hash_shift);
}

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
    INIT_HLIST_BL_NODE(&dentry->d_hash);
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
    return __d_lookup(parent, name);
}
EXPORT_SYMBOL(d_lookup);

/**
 *  d_unhashed -    is dentry hashed
 *  @dentry: entry to check
 *
 *  Returns true if the dentry passed is not currently hashed.
 */
static inline int
d_unhashed(const struct dentry *dentry)
{
    return hlist_bl_unhashed(&dentry->d_hash);
}

static inline int
dentry_string_cmp(const unsigned char *cs,
                  const unsigned char *ct,
                  unsigned tcount)
{
    do {
        if (*cs != *ct)
            return 1;
        cs++;
        ct++;
        tcount--;
    } while (tcount);
    return 0;
}

static inline int
dentry_cmp(const struct dentry *dentry,
           const unsigned char *ct,
           unsigned tcount)
{
    /*
     * Be careful about RCU walk racing with rename:
     * use 'READ_ONCE' to fetch the name pointer.
     *
     * NOTE! Even if a rename will mean that the length
     * was not loaded atomically, we don't care. The
     * RCU walk will check the sequence count eventually,
     * and catch it. And we won't overrun the buffer,
     * because we're reading the name pointer atomically,
     * and a dentry name is guaranteed to be properly
     * terminated with a NUL byte.
     *
     * End result: even if 'len' is wrong, we'll exit
     * early because the data cannot match (there can
     * be no NUL in the ct/tcount data)
     */
    const unsigned char *cs = READ_ONCE(dentry->d_name.name);

    return dentry_string_cmp(cs, ct, tcount);
}

struct dentry *
__d_lookup(const struct dentry *parent, const struct qstr *name)
{
    struct dentry *dentry;
    struct hlist_bl_node *node;
    u64 hashlen = name->hash_len;
    struct hlist_bl_head *b = d_hash(hashlen_hash(hashlen));

    hlist_bl_for_each_entry(dentry, node, b, d_hash) {
        if (dentry->d_parent != parent)
            continue;
        if (d_unhashed(dentry))
            continue;
        if (dentry->d_name.hash_len != hashlen)
            continue;
        if (dentry_cmp(dentry, name->name, hashlen_len(hashlen)) != 0)
            continue;

        return dentry;
    }

    return NULL;
}
EXPORT_SYMBOL(__d_lookup);

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
__d_rehash(struct dentry *entry)
{
    struct hlist_bl_head *b = d_hash(entry->d_name.hash);
    hlist_bl_add_head(&entry->d_hash, b);
}

static inline void
__d_add(struct dentry *dentry, struct inode *inode)
{
    BUG_ON(inode);
    __d_rehash(dentry);
}

/**
 * d_add - add dentry to hash queues
 * @entry: dentry to add
 * @inode: The inode to attach to this dentry
 *
 * This adds the entry to the hash queues and initializes @inode.
 * The entry was actually filled in earlier during d_alloc().
 */
void
d_add(struct dentry *entry, struct inode *inode)
{
    __d_add(entry, inode);
}
EXPORT_SYMBOL(d_add);

static void
dcache_init(void)
{
    dentry_cache = KMEM_CACHE_USERCOPY(dentry,
                                       SLAB_RECLAIM_ACCOUNT|SLAB_PANIC|
                                       SLAB_MEM_SPREAD|SLAB_ACCOUNT,
                                       d_iname);

    dentry_hashtable =
        alloc_large_system_hash("Dentry cache",
                                sizeof(struct hlist_bl_head),
                                13,
                                &d_hash_shift,
                                NULL);

    d_hash_shift = 32 - d_hash_shift;
}

static int
init_module(void)
{
    printk("module[dcache]: init begin ...\n");

    names_cachep =
        kmem_cache_create_usercopy("names_cache", PATH_MAX, 0,
                                   SLAB_HWCACHE_ALIGN|SLAB_PANIC,
                                   0, PATH_MAX, NULL);

    dcache_init();
    printk("module[dcache]: init end!\n");
    return 0;
}
