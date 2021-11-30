// SPDX-License-Identifier: GPL-2.0

#include <fs.h>
#include <slab.h>
#include <errno.h>
#include <blkdev.h>
#include <dcache.h>
#include <export.h>

static LIST_HEAD(super_blocks);

static int
set_bdev_super(struct super_block *s, void *data)
{
    s->s_bdev = data;
    s->s_dev = s->s_bdev->bd_dev;
    return 0;
}

static int
test_bdev_super(struct super_block *s, void *data)
{
    return (void *)s->s_bdev == data;
}

static struct super_block *
alloc_super(struct file_system_type *type, int flags)
{
    struct super_block *s;
    s = kzalloc(sizeof(struct super_block), GFP_USER);
    if (!s)
        return NULL;
    return s;
}

struct super_block *
sget(struct file_system_type *type,
     int (*test)(struct super_block *,void *),
     int (*set)(struct super_block *,void *),
     int flags,
     void *data)
{
    int err;
    struct super_block *old;
    struct super_block *s = NULL;

    if (test) {
        hlist_for_each_entry(old, &type->fs_supers, s_instances) {
            if (!test(old, data))
                continue;
            panic("already exist!");
        }
    }

    s = alloc_super(type, (flags & ~SB_SUBMOUNT));
    if (!s)
        panic("bad alloc!");

    err = set(s, data);
    if (err)
        return ERR_PTR(err);

    s->s_type = type;
    strlcpy(s->s_id, type->name, sizeof(s->s_id));
    list_add_tail(&s->s_list, &super_blocks);
    hlist_add_head(&s->s_instances, &type->fs_supers);
    get_filesystem(type);
    return s;
}

struct dentry *
mount_bdev(struct file_system_type *fs_type,
           int flags, const char *dev_name,
           int (*fill_super)(struct super_block *, void *, int))
{
    int error = 0;
    struct super_block *s;
    struct block_device *bdev;
    fmode_t mode = FMODE_READ | FMODE_EXCL;

    if (!(flags & SB_RDONLY))
        mode |= FMODE_WRITE;

    printk("%s: %s %x\n", __func__, dev_name, flags);
    bdev = blkdev_get_by_path(dev_name, mode, fs_type);
    if (IS_ERR(bdev))
        return ERR_CAST(bdev);

    s = sget(fs_type, test_bdev_super, set_bdev_super, flags|SB_NOSEC, bdev);
    if (IS_ERR(s))
        panic("bad super!");

    s->s_mode = mode;
    snprintf(s->s_id, sizeof(s->s_id), "%pg", bdev);
    sb_set_blocksize(s, block_size(bdev));

    error = fill_super(s, NULL, flags & SB_SILENT ? 1 : 0);
    if (error)
        panic("can not fill_super!");

    s->s_flags |= SB_ACTIVE;
    bdev->bd_super = s;
    return dget(s->s_root);
}
EXPORT_SYMBOL(mount_bdev);
