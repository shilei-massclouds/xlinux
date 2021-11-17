/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_FS_H
#define _LINUX_FS_H

#include <path.h>

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
    void *s_fs_info; /* Proposed s_fs_info */
    unsigned int sb_flags;      /* Proposed superblock flags (SB_*) */
    unsigned int sb_flags_mask; /* Superblock flags that were changed */
    enum fs_context_purpose purpose:8;
};

struct super_block {
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

#endif /* _LINUX_FS_H */
