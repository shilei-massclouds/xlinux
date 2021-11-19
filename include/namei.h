/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_NAMEI_H
#define _LINUX_NAMEI_H

#include <fs.h>
#include <dcache.h>

/* Special value used to indicate 
 * openat should use the current working directory. */
#define AT_FDCWD    -100

#define LOOKUP_DIRECTORY    0x0002  /* require a directory */
#define LOOKUP_REVAL        0x0020  /* tell ->d_revalidate() to trust no cache */
#define LOOKUP_RCU          0x0040  /* RCU pathwalk mode; semi-internal */
#define LOOKUP_ROOT         0x2000
#define LOOKUP_IN_ROOT      0x100000 /* Treat dirfd as fs root. */

struct dentry *
kern_path_create(int dfd, const char *pathname,
                 struct path *path, unsigned int lookup_flags);

#endif /* _LINUX_NAMEI_H */
