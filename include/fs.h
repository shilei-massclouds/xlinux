/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_FS_H
#define _LINUX_FS_H

#include <list.h>
#include <path.h>
#include <types.h>

#define SB_ACTIVE   (1<<30)

struct filename {
    const char *name;   /* pointer to actual string */
    const char iname[];
};

extern struct kmem_cache *names_cachep;

#define __getname() kmem_cache_alloc(names_cachep, GFP_KERNEL)

struct fs_context;

struct fs_context_operations {
    int (*get_tree)(struct fs_context *fc);
};

enum fs_context_purpose {
    FS_CONTEXT_FOR_MOUNT,       /* New superblock for explicit mount */
    FS_CONTEXT_FOR_SUBMOUNT,    /* New superblock for automatic submount */
    FS_CONTEXT_FOR_RECONFIGURE, /* Superblock reconfiguration (remount) */
};

struct fs_context {
    const struct fs_context_operations *ops;
    struct file_system_type *fs_type;
    struct dentry *root; /* The root and superblock */
    void *s_fs_info; /* Proposed s_fs_info */
    unsigned int sb_flags;      /* Proposed superblock flags (SB_*) */
    unsigned int sb_flags_mask; /* Superblock flags that were changed */
    enum fs_context_purpose purpose:8;
};

struct super_block;

struct super_operations {
    struct inode *(*alloc_inode)(struct super_block *sb);
};

struct super_block {
    dev_t s_dev;    /* search index; _not_ kdev_t */
    struct dentry *s_root;
    unsigned long s_flags;
    void *s_fs_info; /* Proposed s_fs_info */
    const struct super_operations *s_op;
    struct list_head    s_inodes;   /* all inodes */
};

struct inode_operations {
    struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);
};

struct inode {
    const struct inode_operations *i_op;
    struct super_block *i_sb;
    unsigned long       i_state;
    struct list_head    i_sb_list;
};

struct file_system_type {
    const char *name;
    int fs_flags;
#define FS_REQUIRES_DEV     1
#define FS_BINARY_MOUNTDATA 2
#define FS_HAS_SUBTYPE      4
#define FS_USERNS_MOUNT     8   /* Can be mounted by userns root */
#define FS_DISALLOW_NOTIFY_PERM 16  /* Disable fanotify permission events */
#define FS_RENAME_DOES_D_MOVE   32768 /* FS will handle d_move() during rename() internally. */
    int (*init_fs_context)(struct fs_context *);
    struct file_system_type * next;
};

struct fs_struct {
    int users;
    int umask;
    int in_exec;
    struct path root, pwd;
};

extern bool rootfs_initialized;

struct fs_context *
fs_context_for_mount(struct file_system_type *fs_type,
                     unsigned int sb_flags);

void
put_fs_context(struct fs_context *fc);

void
set_fs_root(struct fs_struct *fs, const struct path *path);

void
set_fs_pwd(struct fs_struct *fs, const struct path *path);

static inline struct file_system_type *
get_filesystem(struct file_system_type *fs)
{
    return fs;
}

int
get_tree_nodev(struct fs_context *fc,
               int (*fill_super)(struct super_block *sb,
                                 struct fs_context *fc));

struct inode *
new_inode(struct super_block *sb);

static inline void
iput(struct inode *inode)
{
}

struct dentry *
simple_lookup(struct inode *dir,
              struct dentry *dentry,
              unsigned int flags);

int
init_mkdir(const char *pathname, umode_t mode);

#endif /* _LINUX_FS_H */
