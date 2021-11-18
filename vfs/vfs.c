/* SPDX-License-Identifier: GPL-2.0-only */

#include <fs.h>
#include <slab.h>
#include <errno.h>
#include <mount.h>
#include <export.h>
#include <kernel.h>
#include <printk.h>

static struct kmem_cache *mnt_cache;

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

    mnt->mnt.mnt_root   = dget(fc->root);

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

static int
init_module(void)
{
    printk("module[vfs]: init begin ...\n");
    mnt_init();
    printk("module[vfs]: init end!\n");
    return 0;
}
