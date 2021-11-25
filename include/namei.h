/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_NAMEI_H
#define _LINUX_NAMEI_H

#include <fs.h>
#include <dcache.h>

/*
 * Type of the last component on LOOKUP_PARENT
 */
enum {LAST_NORM, LAST_ROOT, LAST_DOT, LAST_DOTDOT};

/* Special value used to indicate
 * openat should use the current working directory. */
#define AT_FDCWD    -100

#define LOOKUP_FOLLOW       0x0001  /* follow links at the end */
#define LOOKUP_DIRECTORY    0x0002  /* require a directory */
#define LOOKUP_ROOT_GRABBED 0x0008
#define LOOKUP_PARENT       0x0010  /* internal use only */
#define LOOKUP_REVAL        0x0020  /* tell ->d_revalidate() to trust no cache */
#define LOOKUP_RCU          0x0040  /* RCU pathwalk mode; semi-internal */
#define LOOKUP_JUMPED       0x1000
#define LOOKUP_ROOT         0x2000
#define LOOKUP_IN_ROOT      0x100000 /* Treat dirfd as fs root. */

struct dentry *
kern_path_create(int dfd, const char *pathname,
                 struct path *path, unsigned int lookup_flags);

int
vfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);

int
vfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev);

#endif /* _LINUX_NAMEI_H */
