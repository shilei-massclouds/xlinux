// SPDX-License-Identifier: GPL-2.0

#include <fs.h>
#include <stat.h>
#include <errno.h>
#include <kernel.h>
#include <fs/ext2.h>
#include <buffer_head.h>

static struct ext2_inode *
ext2_get_inode(struct super_block *sb, ino_t ino, struct buffer_head **p)
{
    unsigned long block;
    unsigned long offset;
    unsigned long block_group;
    struct buffer_head *bh;
    struct ext2_group_desc *gdp;

    block_group = (ino - 1) / EXT2_INODES_PER_GROUP(sb);
    gdp = ext2_get_group_desc(sb, block_group, NULL);
    if (!gdp)
        panic("bad group desc!");

    /*
     * Figure out the offset within the block group inode table
     */
    offset = ((ino - 1) % EXT2_INODES_PER_GROUP(sb)) * EXT2_INODE_SIZE(sb);
    block = gdp->bg_inode_table + (offset >> EXT2_BLOCK_SIZE_BITS(sb));
    if (!(bh = sb_bread_unmovable(sb, block)))
        panic("bad io!");

    printk("%s: ino(%u) bg_inode_table(%lu) offset(%lu) block(%lu)\n",
           __func__, ino, gdp->bg_inode_table, offset, block);

    *p = bh;
    offset &= (EXT2_BLOCK_SIZE(sb) - 1);
    return (struct ext2_inode *) (bh->b_data + offset);
}

struct inode *ext2_iget(struct super_block *sb, unsigned long ino)
{
    struct inode *inode;
    struct ext2_inode *raw_inode;
    struct buffer_head *bh = NULL;

    inode = iget_locked(sb, ino);
    if (!inode)
        return ERR_PTR(-ENOMEM);
    if (!(inode->i_state & I_NEW))
        return inode;

    raw_inode = ext2_get_inode(inode->i_sb, ino, &bh);
    if (IS_ERR(raw_inode))
        panic("bad inode!");

    inode->i_mode = raw_inode->i_mode;
    inode->i_size = raw_inode->i_size;
    inode->i_blocks = raw_inode->i_blocks;

    if (S_ISREG(inode->i_mode)) {
        panic("Regular file!");
    } else if (S_ISDIR(inode->i_mode)) {
        printk("%s: ino(%lu)\n", __func__, ino);
        inode->i_op = &ext2_dir_inode_operations;
    } else {
        panic("unknown file!");
    }

    return inode;
}
