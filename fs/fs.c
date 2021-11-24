// SPDX-License-Identifier: GPL-2.0+

#include <fs.h>
#include <slab.h>
#include <errno.h>
#include <mount.h>
#include <export.h>
#include <printk.h>

static u32 unnamed_dev_ida;

static struct kmem_cache *inode_cachep;

void
set_fs_root(struct fs_struct *fs, const struct path *path)
{
    struct path old_root;

    old_root = fs->root;
    fs->root = *path;
    if (old_root.dentry)
        path_put(&old_root);
}
EXPORT_SYMBOL(set_fs_root);

void
set_fs_pwd(struct fs_struct *fs, const struct path *path)
{
    struct path old_pwd;

    old_pwd = fs->pwd;
    fs->pwd = *path;
    if (old_pwd.dentry)
        path_put(&old_pwd);
}
EXPORT_SYMBOL(set_fs_pwd);

static struct fs_context *
alloc_fs_context(struct file_system_type *fs_type,
                 struct dentry *reference,
                 unsigned int sb_flags,
                 unsigned int sb_flags_mask,
                 enum fs_context_purpose purpose)
{
    int (*init_fs_context)(struct fs_context *);
    struct fs_context *fc;

    fc = kzalloc(sizeof(struct fs_context), GFP_KERNEL);
    if (!fc)
        return ERR_PTR(-ENOMEM);

    fc->purpose     = purpose;
    fc->sb_flags    = sb_flags;
    fc->sb_flags_mask = sb_flags_mask;
    fc->fs_type     = get_filesystem(fs_type);

    init_fs_context = fc->fs_type->init_fs_context;
    if (!init_fs_context)
        panic("no init_fs_context!");

    if (init_fs_context(fc) < 0)
        panic("cannot init_fs_context!");

    return fc;
}

struct fs_context *
fs_context_for_mount(struct file_system_type *fs_type,
                     unsigned int sb_flags)
{
    return alloc_fs_context(fs_type, NULL,
                            sb_flags, 0, FS_CONTEXT_FOR_MOUNT);
}
EXPORT_SYMBOL(fs_context_for_mount);

void
put_fs_context(struct fs_context *fc)
{
    /* Todo: */
}
EXPORT_SYMBOL(put_fs_context);

static struct super_block *
alloc_super(struct file_system_type *type, int flags)
{
    struct super_block *s;
    s = kzalloc(sizeof(struct super_block), GFP_USER);
    if (!s)
        return NULL;

    INIT_LIST_HEAD(&s->s_inodes);
    return s;
}

struct super_block *
sget_fc(struct fs_context *fc,
        int (*set)(struct super_block *, struct fs_context *))
{
    struct super_block *s = NULL;

    s = alloc_super(fc->fs_type, fc->sb_flags);
    if (!s)
        return ERR_PTR(-ENOMEM);

    s->s_fs_info = fc->s_fs_info;
    if (set(s, fc))
        panic("cannot set!");

    return s;
}

int
get_anon_bdev(dev_t *p)
{
    *p = MKDEV(0, unnamed_dev_ida++);
    return 0;
}
EXPORT_SYMBOL(get_anon_bdev);

int
set_anon_super(struct super_block *s, void *data)
{
    return get_anon_bdev(&s->s_dev);
}
EXPORT_SYMBOL(set_anon_super);

int
set_anon_super_fc(struct super_block *sb, struct fs_context *fc)
{
    return set_anon_super(sb, NULL);
}
EXPORT_SYMBOL(set_anon_super_fc);

int
vfs_get_super(struct fs_context *fc,
              int (*fill_super)(struct super_block *sb,
                                struct fs_context *fc))
{
    struct super_block *sb;
    sb = sget_fc(fc, set_anon_super_fc);
    if (IS_ERR(sb))
        return PTR_ERR(sb);

    BUG_ON(sb->s_root);
    if (fill_super(sb, fc))
        panic("cannot fill super!");

    sb->s_flags |= SB_ACTIVE;
    fc->root = dget(sb->s_root);
    return 0;
}

int
get_tree_nodev(struct fs_context *fc,
               int (*fill_super)(struct super_block *sb,
                                 struct fs_context *fc))
{
    return vfs_get_super(fc, fill_super);
}
EXPORT_SYMBOL(get_tree_nodev);

int
inode_init_always(struct super_block *sb, struct inode *inode)
{
    inode->i_sb = sb;
    return 0;
}

static struct inode *
alloc_inode(struct super_block *sb)
{
    struct inode *inode;
    const struct super_operations *ops = sb->s_op;

    if (ops->alloc_inode)
        inode = ops->alloc_inode(sb);
    else
        inode = kmem_cache_alloc(inode_cachep, GFP_KERNEL);

    if (!inode)
        return NULL;

    BUG_ON(unlikely(inode_init_always(sb, inode)));
    return inode;
}

struct inode *
new_inode_pseudo(struct super_block *sb)
{
    struct inode *inode = alloc_inode(sb);

    if (inode) {
        inode->i_state = 0;
        INIT_LIST_HEAD(&inode->i_sb_list);
    }
    return inode;
}

void
inode_sb_list_add(struct inode *inode)
{
    list_add(&inode->i_sb_list, &inode->i_sb->s_inodes);
}
EXPORT_SYMBOL(inode_sb_list_add);

struct inode *
new_inode(struct super_block *sb)
{
    struct inode *inode;

    inode = new_inode_pseudo(sb);
    if (inode)
        inode_sb_list_add(inode);
    return inode;
}
EXPORT_SYMBOL(new_inode);

/*
struct dentry *
simple_lookup(struct inode *dir,
              struct dentry *dentry,
              unsigned int flags)
{
    if (dentry->d_name.len > NAME_MAX)
        return ERR_PTR(-ENAMETOOLONG);
    if (!dentry->d_sb->s_d_op)
        d_set_d_op(dentry, &simple_dentry_operations);
    d_add(dentry, NULL);
    return NULL;
}
EXPORT_SYMBOL(simple_lookup);
*/

static void
init_once(void *foo)
{
}

void
inode_init(void)
{
    /* inode slab cache */
    inode_cachep = kmem_cache_create("inode_cache",
                                     sizeof(struct inode),
                                     0,
                                     (SLAB_RECLAIM_ACCOUNT|SLAB_PANIC|
                                      SLAB_MEM_SPREAD|SLAB_ACCOUNT),
                                     init_once);
}

static int
init_module(void)
{
    printk("module[fs]: init begin ...\n");
    BUG_ON(!slab_is_available());
    inode_init();
    mnt_init();
    BUG_ON(!names_cachep);
    printk("module[fs]: init end!\n");
    return 0;
}
