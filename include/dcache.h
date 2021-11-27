/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_DCACHE_H
#define _LINUX_DCACHE_H

#include <fs.h>
#include <list_bl.h>

#define DNAME_INLINE_LEN 32 /* 192 bytes */

#define HASH_LEN_DECLARE u32 hash; u32 len
#define bytemask_from_count(cnt)   (~(~0ul << (cnt)*8))

struct qstr {
    union {
        struct {
            HASH_LEN_DECLARE;
        };
        u64 hash_len;
    };
    const unsigned char *name;
};

#define QSTR_INIT(n,l) { { { .len = l } }, .name = n }

struct dentry {
    struct hlist_bl_node d_hash;    /* lookup hash list */
    struct dentry *d_parent;    /* parent directory */
    struct qstr d_name;
    struct inode *d_inode;  /* Where the name belongs to - NULL is negative */

    unsigned char d_iname[DNAME_INLINE_LEN];    /* small names */
    struct super_block *d_sb;   /* The root of the dentry tree */
    struct list_head d_child;   /* child of parent list */
    struct list_head d_subdirs; /* our children */
};

static inline struct dentry *
dget(struct dentry *dentry)
{
    return dentry;
}

struct dentry *
d_make_root(struct inode *root_inode);

struct dentry *
d_lookup(const struct dentry *parent, const struct qstr *name);

struct dentry *
__d_lookup(const struct dentry *parent, const struct qstr *name);

struct dentry *
d_alloc(struct dentry * parent, const struct qstr *name);

void
d_instantiate(struct dentry *entry, struct inode * inode);

static inline struct inode *
d_backing_inode(const struct dentry *upper)
{
    return upper->d_inode;
}

void
d_add(struct dentry *entry, struct inode *inode);

#endif /* _LINUX_DCACHE_H */
