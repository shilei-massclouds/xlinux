// SPDX-License-Identifier: GPL-2.0

#include <slab.h>
#include <stat.h>
#include <errno.h>
#include <namei.h>
#include <dcache.h>
#include <export.h>
#include <limits.h>
#include <string.h>
#include <current.h>

#define EMBEDDED_NAME_MAX (PATH_MAX - offsetof(struct filename, iname))

struct nameidata {
    struct path path;
    struct qstr last;
    struct path root;
    struct inode *inode; /* path.dentry.d_inode */
    unsigned int flags;
    int last_type;
    struct filename *name;
    int dfd;
};

struct filename *
getname_kernel(const char *filename)
{
    struct filename *result;
    int len = strlen(filename) + 1;

    result = __getname();
    if (unlikely(!result))
        return ERR_PTR(-ENOMEM);

    if (len <= EMBEDDED_NAME_MAX) {
        result->name = (char *)result->iname;
    } else if (len <= PATH_MAX) {
        const size_t size = offsetof(struct filename, iname[1]);
        struct filename *tmp;

        tmp = kmalloc(size, GFP_KERNEL);
        if (unlikely(!tmp)) {
            return ERR_PTR(-ENOMEM);
        }
        tmp->name = (char *)result;
        result = tmp;
    } else {
        return ERR_PTR(-ENAMETOOLONG);
    }

    memcpy((char *)result->name, filename, len);
    return result;
}

static const char *
path_init(struct nameidata *nd, unsigned flags)
{
    const char *s = nd->name->name;

    if (!*s)
        flags &= ~LOOKUP_RCU;

    if (flags & LOOKUP_ROOT)
        panic("no LOOKUP_ROOT!");

    nd->root.mnt = NULL;
    nd->path.mnt = NULL;
    nd->path.dentry = NULL;

    BUG_ON(*s == '/');

    /* Relative pathname -- get the starting-point it is relative to. */
    if (nd->dfd == AT_FDCWD) {
        if (flags & LOOKUP_RCU) {
            struct fs_struct *fs = current->fs;
            nd->path = fs->pwd;
            nd->inode = nd->path.dentry->d_inode;
        } else {
            panic("no LOOKUP_RCU!");
        }
    } else {
        panic("no AT_FDCMD!");
    }

    return s;
}

static int
link_path_walk(const char *name, struct nameidata *nd)
{
    int err;
    int depth = 0; // depth <= nd->depth

    nd->last_type = LAST_ROOT;
    nd->flags |= LOOKUP_PARENT;
    if (IS_ERR(name))
        return PTR_ERR(name);
    while (*name=='/')
        name++;
    if (!*name)
        return 0;

    nd->last.name = name;
    nd->last.len = strlen(name);
    nd->last_type = LAST_NORM;
    nd->flags &= ~LOOKUP_PARENT;
    return 0;
}

/* Returns 0 and nd will be valid on success; Retuns error, otherwise. */
static int
path_parentat(struct nameidata *nd, unsigned flags, struct path *parent)
{
    const char *s = path_init(nd, flags);
    int err = link_path_walk(s, nd);
    if (!err) {
        *parent = nd->path;
        nd->path.mnt = NULL;
        nd->path.dentry = NULL;
    }
    return err;
}

static void
set_nameidata(struct nameidata *p, int dfd, struct filename *name)
{
    p->dfd = dfd;
    p->name = name;
}

static struct filename *
filename_parentat(int dfd, struct filename *name,
                  unsigned int flags, struct path *parent,
                  struct qstr *last, int *type)
{
    int retval;
    struct nameidata nd;

    if (IS_ERR(name))
        return name;

    set_nameidata(&nd, dfd, name);
    retval = path_parentat(&nd, flags | LOOKUP_RCU, parent);
    if (likely(!retval)) {
        *last = nd.last;
        *type = nd.last_type;
    } else {
        name = ERR_PTR(retval);
    }

    return name;
}

static struct dentry *
lookup_dcache(const struct qstr *name,
              struct dentry *dir,
              unsigned int flags)
{
    struct dentry *dentry = d_lookup(dir, name);
    return dentry;
}

static struct dentry *
__lookup_hash(const struct qstr *name,
              struct dentry *base,
              unsigned int flags)
{
    struct dentry *dentry;

    dentry = d_alloc(base, name);
    if (unlikely(!dentry))
        return ERR_PTR(-ENOMEM);

    return dentry;
}

static struct dentry *
filename_create(int dfd, struct filename *name,
                struct path *path, unsigned int lookup_flags)
{
    int type;
    struct qstr last;
    struct dentry *dentry;
    bool is_dir = (lookup_flags & LOOKUP_DIRECTORY);

    /*
     * Note that only LOOKUP_REVAL and LOOKUP_DIRECTORY matter here. Any
     * other flags passed in are ignored!
     */
    lookup_flags &= LOOKUP_REVAL;

    name = filename_parentat(dfd, name, lookup_flags, path, &last, &type);
    if (IS_ERR(name))
        return ERR_CAST(name);

    dentry = __lookup_hash(&last, path->dentry, lookup_flags);
    if (IS_ERR(dentry))
        panic("cannot lookup hash!");

    return dentry;
}

struct dentry *
kern_path_create(int dfd, const char *pathname,
                 struct path *path, unsigned int lookup_flags)
{
    return filename_create(dfd, getname_kernel(pathname),
                           path, lookup_flags);
}
EXPORT_SYMBOL(kern_path_create);

int
vfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
    if (!dir->i_op->mkdir)
        return -EPERM;

    mode &= (S_IRWXUGO|S_ISVTX);
    return dir->i_op->mkdir(dir, dentry, mode);
}
EXPORT_SYMBOL(vfs_mkdir);
