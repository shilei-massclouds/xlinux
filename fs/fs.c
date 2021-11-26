// SPDX-License-Identifier: GPL-2.0+

#include <fs.h>
#include <bug.h>
#include <slab.h>
#include <errno.h>
#include <mount.h>
#include <export.h>
#include <printk.h>
#include <string.h>

struct file_system_type *file_systems;

static struct kmem_cache *inode_cachep;
static u32 unnamed_dev_ida;

enum legacy_fs_param {
    LEGACY_FS_UNSET_PARAMS,
    LEGACY_FS_MONOLITHIC_PARAMS,
    LEGACY_FS_INDIVIDUAL_PARAMS,
};

struct legacy_fs_context {
    char *legacy_data;  /* Data page for legacy filesystems */
    size_t data_size;
    enum legacy_fs_param param_type;
};

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

/*
 * Get a mountable root with the legacy mount command.
 */
static int
legacy_get_tree(struct fs_context *fc)
{
    struct super_block *sb;
    struct dentry *root;

    root = fc->fs_type->mount(fc->fs_type, fc->sb_flags, fc->source);
    if (IS_ERR(root))
        return PTR_ERR(root);

    sb = root->d_sb;
    BUG_ON(!sb);

    fc->root = root;
    return 0;
}

const struct fs_context_operations legacy_fs_context_ops = {
    .get_tree = legacy_get_tree,
};

/*
 * Initialise a legacy context for a filesystem that doesn't support
 * fs_context.
 */
static int
legacy_init_fs_context(struct fs_context *fc)
{
    fc->ops = &legacy_fs_context_ops;
    return 0;
}

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
        init_fs_context = legacy_init_fs_context;

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

static struct file_system_type **
find_filesystem(const char *name, unsigned len)
{
    struct file_system_type **p;
    for (p = &file_systems; *p; p = &(*p)->next)
        if (strncmp((*p)->name, name, len) == 0 && !(*p)->name[len])
            break;
    return p;
}

int
register_filesystem(struct file_system_type *fs)
{
    struct file_system_type ** p;

    BUG_ON(strchr(fs->name, '.'));
    if (fs->next)
        return -EBUSY;

    p = find_filesystem(fs->name, strlen(fs->name));
    if (*p)
        return -EBUSY;

    *p = fs;
    return 0;
}
EXPORT_SYMBOL(register_filesystem);

int
get_filesystem_list(char *buf)
{
    int len = 0;
    struct file_system_type * tmp;

    tmp = file_systems;
    while (tmp && len < PAGE_SIZE - 80) {
        len += sprintf(buf+len, "%s\t%s\n",
                       (tmp->fs_flags & FS_REQUIRES_DEV) ? "" : "nodev",
                       tmp->name);
        tmp = tmp->next;
    }
    return len;
}
EXPORT_SYMBOL(get_filesystem_list);

static struct file_system_type *
__get_fs_type(const char *name, int len)
{
    return *(find_filesystem(name, len));
}

struct file_system_type *
get_fs_type(const char *name)
{
    struct file_system_type *fs;
    const char *dot = strchr(name, '.');
    int len = dot ? dot - name : strlen(name);

    fs = __get_fs_type(name, len);
    BUG_ON(!fs);
    return fs;
}
EXPORT_SYMBOL(get_fs_type);

struct dentry *
mount_bdev(struct file_system_type *fs_type,
           int flags, const char *dev_name,
           int (*fill_super)(struct super_block *, void *, int))
{
    struct block_device *bdev;
    fmode_t mode = FMODE_READ | FMODE_EXCL;

    if (!(flags & SB_RDONLY))
        mode |= FMODE_WRITE;

    bdev = blkdev_get_by_path(dev_name, mode, fs_type);
    if (IS_ERR(bdev))
        return ERR_CAST(bdev);

    panic("%s: ", __func__);
}
EXPORT_SYMBOL(mount_bdev);

static int
pseudo_fs_fill_super(struct super_block *s, struct fs_context *fc)
{
    panic("%s: ", __func__);
}

static int
pseudo_fs_get_tree(struct fs_context *fc)
{
    return get_tree_nodev(fc, pseudo_fs_fill_super);
}

static const struct fs_context_operations pseudo_fs_context_ops = {
    .get_tree = pseudo_fs_get_tree,
};

struct pseudo_fs_context *
init_pseudo(struct fs_context *fc, unsigned long magic)
{
    struct pseudo_fs_context *ctx;

    ctx = kzalloc(sizeof(struct pseudo_fs_context), GFP_KERNEL);
    if (likely(ctx)) {
        ctx->magic = magic;
        fc->ops = &pseudo_fs_context_ops;
        fc->sb_flags |= SB_NOUSER;
    }
    return ctx;
}
EXPORT_SYMBOL(init_pseudo);

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
    bdev_cache_init();
    printk("module[fs]: init end!\n");
    return 0;
}
