/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _EXT2_H_
#define _EXT2_H_

/*
 * Special inode numbers
 */
#define EXT2_BAD_INO         1  /* Bad blocks inode */
#define EXT2_ROOT_INO        2  /* Root inode */

#define EXT2_INODES_PER_GROUP(s)    (EXT2_SB(s)->s_inodes_per_group)

struct ext2_super_block {
    u32 s_inodes_per_group; /* # Inodes per group */
};

struct ext2_inode_info {
    u32 i_data[15];

    struct inode vfs_inode;
};

struct ext2_sb_info {
    unsigned long s_sb_block;
    unsigned long s_inodes_per_group;   /* #Inodes in a group */
};

struct inode *ext2_iget(struct super_block *sb, unsigned long ino);

static inline struct ext2_sb_info *EXT2_SB(struct super_block *sb)
{
    return sb->s_fs_info;
}

#endif /* _EXT2_H_ */
