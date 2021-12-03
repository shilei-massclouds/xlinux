// SPDX-License-Identifier: GPL-2.0

#include <fs.h>
#include <errno.h>
#include <kernel.h>
#include <fs/ext2.h>
#include <buffer_head.h>

static struct ext2_inode *
ext2_get_inode(struct super_block *sb, ino_t ino, struct buffer_head **p)
{
    unsigned long block_group;

    block_group = (ino - 1) / EXT2_INODES_PER_GROUP(sb);
    panic("%s: ino(%lu) block_group(%lu)",
          __func__, ino, block_group);
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

    panic("%s:", __func__);
    return inode;
}
