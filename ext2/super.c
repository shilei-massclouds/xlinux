// SPDX-License-Identifier: GPL-2.0-only

#include <fs.h>
#include <slab.h>
#include <stat.h>
#include <errno.h>
#include <dcache.h>
#include <export.h>
#include <fs/ext2.h>
#include <buffer_head.h>

static struct kmem_cache *ext2_inode_cachep;

static struct inode *ext2_alloc_inode(struct super_block *sb)
{
    struct ext2_inode_info *ei;
    ei = kmem_cache_alloc(ext2_inode_cachep, GFP_KERNEL);
    if (!ei)
        return NULL;

    return &ei->vfs_inode;
}

static const struct super_operations ext2_sops = {
    .alloc_inode    = ext2_alloc_inode,
};

static unsigned long
get_sb_block(void **data)
{
    BUG_ON((*data));

    return 1;   /* Default location */
}

static int
ext2_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root;
    struct buffer_head *bh;
    struct ext2_sb_info *sbi;
    struct ext2_super_block *es;
    unsigned long logic_sb_block;
    unsigned long offset = 0;
    int blocksize = BLOCK_SIZE;
    unsigned long sb_block = get_sb_block(&data);

    sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
    if (!sbi)
        panic("out of memory!");

    sb->s_fs_info = sbi;
    sbi->s_sb_block = sb_block;

    blocksize = sb_min_blocksize(sb, BLOCK_SIZE);
    if (!blocksize)
        panic("error: unable to set blocksize");

    if (blocksize != BLOCK_SIZE) {
        logic_sb_block = (sb_block*BLOCK_SIZE) / blocksize;
        offset = (sb_block*BLOCK_SIZE) % blocksize;
    } else {
        logic_sb_block = sb_block;
    }

    printk("############### %s: (%d) ...\n", __func__, logic_sb_block);
    if (!(bh = sb_bread_unmovable(sb, logic_sb_block)))
        panic("error: unable to read superblock");
    printk("############### %s: (%d) ok!\n", __func__, logic_sb_block);

    es = (struct ext2_super_block *) (((char *)bh->b_data) + offset);
    sbi->s_inodes_per_group = es->s_inodes_per_group;

    printk("%s: step1\n", __func__);
    sb->s_op = &ext2_sops;

    root = ext2_iget(sb, EXT2_ROOT_INO);
    printk("%s: step2\n", __func__);
    if (IS_ERR(root))
        panic("can not get root inode! (%d)!", PTR_ERR(root));

    if (!S_ISDIR(root->i_mode) || !root->i_blocks || !root->i_size)
        panic("error: corrupt root inode, run e2fsck");

    printk("%s: step3\n", __func__);
    sb->s_root = d_make_root(root);
    if (!sb->s_root)
        panic("no root!");

    /*
    if (ext2_setup_super (sb, es, sb_rdonly(sb)))
        sb->s_flags |= SB_RDONLY;
    ext2_write_super(sb);
    */

    panic("%s: ", __func__);
}

static struct dentry *
ext2_mount(struct file_system_type *fs_type, int flags, const char *dev_name)
{
    printk("%s: dev_name(%s) flags(%x)\n", __func__, dev_name, flags);
    return mount_bdev(fs_type, flags, dev_name, ext2_fill_super);
}

static struct file_system_type ext2_fs_type = {
    .name       = "ext2",
    .mount      = ext2_mount,
    .fs_flags   = FS_REQUIRES_DEV,
};

static void init_once(void *foo)
{
    struct ext2_inode_info *ei = (struct ext2_inode_info *) foo;
    inode_init_once(&ei->vfs_inode);
}

static int init_inodecache(void)
{
    ext2_inode_cachep =
        kmem_cache_create_usercopy("ext2_inode_cache",
                                   sizeof(struct ext2_inode_info), 0,
                                   (SLAB_RECLAIM_ACCOUNT|
                                    SLAB_MEM_SPREAD|
                                    SLAB_ACCOUNT),
                                   offsetof(struct ext2_inode_info, i_data),
                                   sizeof_field(struct ext2_inode_info, i_data),
                                   init_once);
    if (ext2_inode_cachep == NULL)
        return -ENOMEM;

    return 0;
}

int init_ext2_fs(void)
{
    int err;

    err = init_inodecache();
    if (err)
        return err;

    err = register_filesystem(&ext2_fs_type);
    if (err)
        panic("can not register ext2 fs!");

    return 0;
}
