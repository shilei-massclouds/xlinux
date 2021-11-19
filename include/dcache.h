/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_DCACHE_H
#define _LINUX_DCACHE_H

#include <fs.h>

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
    struct dentry *d_parent; /* parent directory */
    struct qstr d_name;
    struct inode *d_inode;   /* Where the name belongs to - NULL is negative */

    unsigned char d_iname[DNAME_INLINE_LEN];    /* small names */
    struct super_block *d_sb;   /* The root of the dentry tree */
};

static inline struct dentry *
dget(struct dentry *dentry)
{
    return dentry;
}

struct dentry *
d_make_root(struct inode *root_inode);

#endif /* _LINUX_DCACHE_H */
