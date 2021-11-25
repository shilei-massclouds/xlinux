/* SPDX-License-Identifier: GPL-2.0-only */

#include <fs.h>
#include <slab.h>
#include <errno.h>
#include <mount.h>
#include <namei.h>
#include <export.h>
#include <kernel.h>
#include <printk.h>
#include <string.h>

static struct kmem_cache *mnt_cache;

int
vfs_parse_fs_string(struct fs_context *fc,
                    const char *key,
                    const char *value,
                    size_t v_size)
{
    if (strcmp(key, "source") == 0) {
        if (fc->source)
            panic("VFS: Multiple sources");
        fc->source = kmemdup_nul(value, v_size, GFP_KERNEL);
        return 0;
    }

    panic("bad vfs params(%s)", key);
    return 0;
}
EXPORT_SYMBOL(vfs_parse_fs_string);

int
vfs_get_tree(struct fs_context *fc)
{
    int error;

    if (fc->root)
        return -EBUSY;

    error = fc->ops->get_tree(fc);
    if (error < 0)
        return error;

    return 0;
}
EXPORT_SYMBOL(vfs_get_tree);

/*
 * create a new mount for userspace and request it to be added into the
 * namespace's tree
 */
static int
do_new_mount(struct path *path,
             const char *fstype,
             int sb_flags,
             int mnt_flags,
             const char *name)
{
    int err;
    struct fs_context *fc;
    struct file_system_type *type;

    if (!fstype)
        return -EINVAL;

    type = get_fs_type(fstype);
    if (!type)
        return -ENODEV;

    BUG_ON(type->fs_flags & FS_HAS_SUBTYPE);

    fc = fs_context_for_mount(type, sb_flags);
    if (IS_ERR(fc))
        return PTR_ERR(fc);

    if (name)
        err = vfs_parse_fs_string(fc, "source", name, strlen(name));

    if (!err)
        err = vfs_get_tree(fc);

    panic("%s: type(%s) source(%s)\n", __func__, type->name, fc->source);
}

int
path_mount(const char *dev_name,
           struct path *path,
           const char *type_page,
           unsigned long flags)
{
    unsigned int sb_flags;
    unsigned int mnt_flags = 0;

    if (flags & MS_RDONLY)
        mnt_flags |= MNT_READONLY;

    sb_flags = flags & (SB_RDONLY | SB_SYNCHRONOUS | SB_MANDLOCK | SB_DIRSYNC |
                        SB_SILENT | SB_POSIXACL | SB_LAZYTIME | SB_I_VERSION);

    return do_new_mount(path, type_page, sb_flags, mnt_flags, dev_name);
}

int
init_mount(const char *dev_name, const char *dir_name,
           const char *type_page, unsigned long flags)
{
    struct path path;
    int ret;

    ret = kern_path(dir_name, LOOKUP_FOLLOW, &path);
    if (ret)
        return ret;

    ret = path_mount(dev_name, &path, type_page, flags);

    printk("### %s: dev(%s) dir(%s) fs(%s) ret(%d)\n",
           __func__, dev_name, dir_name, type_page, ret);

    path_put(&path);
    panic("Reach here!");
    return ret;
}
EXPORT_SYMBOL(init_mount);

static struct mount *
alloc_vfsmnt(const char *name)
{
    struct mount *mnt;
    mnt = kmem_cache_zalloc(mnt_cache, GFP_KERNEL);
    return mnt;
}

struct vfsmount *
vfs_create_mount(struct fs_context *fc)
{
    struct mount *mnt;

    if (!fc->root)
        return ERR_PTR(-EINVAL);

    mnt = alloc_vfsmnt("none");
    if (!mnt)
        return ERR_PTR(-ENOMEM);

    mnt->mnt.mnt_sb   = fc->root->d_sb;
    mnt->mnt.mnt_root = dget(fc->root);
    return &mnt->mnt;
}

struct vfsmount *
fc_mount(struct fs_context *fc)
{
    int err = vfs_get_tree(fc);
    if (err) {
        return ERR_PTR(err);
    }
    return vfs_create_mount(fc);
}
EXPORT_SYMBOL(fc_mount);

struct vfsmount *
vfs_kern_mount(struct file_system_type *type,
               int flags, const char *name, void *data)
{
    struct fs_context *fc;
    struct vfsmount *mnt;
    int ret = 0;

    if (!type)
        return ERR_PTR(-EINVAL);

    fc = fs_context_for_mount(type, flags);
    if (IS_ERR(fc))
        return ERR_CAST(fc);

    mnt = fc_mount(fc);
    if (ret)
        mnt = ERR_PTR(ret);

    put_fs_context(fc);
    return mnt;
}
EXPORT_SYMBOL(vfs_kern_mount);

void
mnt_init(void)
{
    mnt_cache = kmem_cache_create("mnt_cache", sizeof(struct mount),0,
                                  SLAB_HWCACHE_ALIGN | SLAB_PANIC, NULL);
}
