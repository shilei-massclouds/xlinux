// SPDX-License-Identifier: GPL-2.0-only

#include <fs.h>
#include <bug.h>
#include <slab.h>
#include <stat.h>
#include <errno.h>
#include <mount.h>
#include <namei.h>
#include <blkdev.h>
#include <export.h>
#include <kdev_t.h>
#include <kernel.h>
#include <printk.h>
#include <string.h>

#define BDEVFS_MAGIC 0x62646576

struct bdev_inode {
    struct block_device bdev;
    struct inode vfs_inode;
};

static struct kmem_cache *bdev_cachep;

struct super_block *blockdev_superblock;
EXPORT_SYMBOL(blockdev_superblock);

static inline unsigned long
hash(dev_t dev)
{
    return MAJOR(dev) + MINOR(dev);
}

static inline struct bdev_inode *
BDEV_I(struct inode *inode)
{
    return container_of(inode, struct bdev_inode, vfs_inode);
}

static int
bdev_test(struct inode *inode, void *data)
{
    return BDEV_I(inode)->bdev.bd_dev == *(dev_t *)data;
}

static int
bdev_set(struct inode *inode, void *data)
{
    BDEV_I(inode)->bdev.bd_dev = *(dev_t *)data;
    return 0;
}

void
unlock_new_inode(struct inode *inode)
{
    BUG_ON(!(inode->i_state & I_NEW));
    inode->i_state &= ~I_NEW & ~I_CREATING;
}
EXPORT_SYMBOL(unlock_new_inode);

struct block_device *
bdget(dev_t dev)
{
    struct block_device *bdev;
    struct inode *inode;

    inode = iget5_locked(blockdev_superblock, hash(dev),
                         bdev_test, bdev_set, &dev);
    if (!inode)
        return NULL;

    bdev = &BDEV_I(inode)->bdev;
    if (inode->i_state & I_NEW) {
        bdev->bd_super = NULL;
        bdev->bd_inode = inode;
        inode->i_mode = S_IFBLK;
        inode->i_rdev = dev;
        inode->i_bdev = bdev;
        unlock_new_inode(inode);
    }
    return bdev;
}

static struct block_device *
bd_acquire(struct inode *inode)
{
    struct block_device *bdev;

    bdev = inode->i_bdev;
    if (bdev)
        return bdev;

    printk("%s: i_rdev(%x)\n", __func__, inode->i_rdev);
    bdev = bdget(inode->i_rdev);
    if (bdev) {
        if (!inode->i_bdev)
            inode->i_bdev = bdev;
    }
    return bdev;
}

struct block_device *
lookup_bdev(const char *pathname)
{
    int error;
    struct path path;
    struct inode *inode;
    struct block_device *bdev;

    if (!pathname || !*pathname)
        return ERR_PTR(-EINVAL);

    printk("%s: pathname(%s)\n", __func__, pathname);
    error = kern_path(pathname, LOOKUP_FOLLOW, &path);
    if (error)
        return ERR_PTR(error);

    inode = d_backing_inode(path.dentry);
    error = -ENOTBLK;
    if (!S_ISBLK(inode->i_mode))
        goto fail;
    error = -ENOMEM;
    bdev = bd_acquire(inode);
    if (!bdev) {
        panic("bad bdev!");
        goto fail;
    }
out:
    path_put(&path);
    return bdev;
fail:
    bdev = ERR_PTR(error);
    goto out;
}
EXPORT_SYMBOL(lookup_bdev);

static int
__blkdev_get(struct block_device *bdev,
             fmode_t mode,
             void *holder,
             int for_part)
{
    panic("%s:", __func__);
}

int
blkdev_get(struct block_device *bdev, fmode_t mode, void *holder)
{
    return __blkdev_get(bdev, mode, holder, 0);
}
EXPORT_SYMBOL(blkdev_get);

struct block_device *
blkdev_get_by_path(const char *path, fmode_t mode, void *holder)
{
    int err;
    struct block_device *bdev;

    printk("%s: mode(%x) path(%s)\n", __func__, mode, path);
    bdev = lookup_bdev(path);
    if (IS_ERR(bdev))
        return bdev;

    err = blkdev_get(bdev, mode, holder);
    if (err)
        return ERR_PTR(err);

    return bdev;
}
EXPORT_SYMBOL(blkdev_get_by_path);

static struct inode *
bdev_alloc_inode(struct super_block *sb)
{
    struct bdev_inode *ei = kmem_cache_alloc(bdev_cachep, GFP_KERNEL);
    if (!ei)
        return NULL;
    return &ei->vfs_inode;
}

static const struct super_operations bdev_sops = {
    .alloc_inode = bdev_alloc_inode,
};

static int
bd_init_fs_context(struct fs_context *fc)
{
    struct pseudo_fs_context *ctx = init_pseudo(fc, BDEVFS_MAGIC);
    if (!ctx)
        return -ENOMEM;
    ctx->ops = &bdev_sops;
    return 0;
}

static void
init_once(void *foo)
{
    struct bdev_inode *ei = (struct bdev_inode *) foo;
    struct block_device *bdev = &ei->bdev;

    memset(bdev, 0, sizeof(*bdev));
    inode_init_once(&ei->vfs_inode);
}

static struct file_system_type bd_type = {
    .name = "bdev",
    .init_fs_context = bd_init_fs_context,
};

void
bdev_cache_init(void)
{
    int err;
    static struct vfsmount *bd_mnt;

    bdev_cachep = kmem_cache_create("bdev_cache",
                                    sizeof(struct bdev_inode),
                                    0,
                                    (SLAB_HWCACHE_ALIGN|
                                     SLAB_RECLAIM_ACCOUNT|
                                     SLAB_MEM_SPREAD|
                                     SLAB_ACCOUNT|
                                     SLAB_PANIC),
                                    init_once);

    err = register_filesystem(&bd_type);
    if (err)
        panic("Cannot register bdev pseudo-fs");
    bd_mnt = kern_mount(&bd_type);
    if (IS_ERR(bd_mnt))
        panic("Cannot create bdev pseudo-fs");
    blockdev_superblock = bd_mnt->mnt_sb;   /* For writeback */
}
