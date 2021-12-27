/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _EXT2_H_
#define _EXT2_H_

#include <fs.h>
#include <types.h>
#include <buffer_head.h>

/*
 * Special inode numbers
 */
#define EXT2_BAD_INO         1  /* Bad blocks inode */
#define EXT2_ROOT_INO        2  /* Root inode */

/*
 * Structure of the super block
 */
struct ext2_super_block {
	u32	s_inodes_count;		/* Inodes count */
	u32	s_blocks_count;		/* Blocks count */
	u32	s_r_blocks_count;	/* Reserved blocks count */
	u32	s_free_blocks_count;	/* Free blocks count */
	u32	s_free_inodes_count;	/* Free inodes count */
	u32	s_first_data_block;	/* First Data Block */
	u32	s_log_block_size;	/* Block size */
	u32	s_log_frag_size;	/* Fragment size */
	u32	s_blocks_per_group;	/* # Blocks per group */
	u32	s_frags_per_group;	/* # Fragments per group */
	u32	s_inodes_per_group;	/* # Inodes per group */
	u32	s_mtime;		/* Mount time */
	u32	s_wtime;		/* Write time */
	u16	s_mnt_count;		/* Mount count */
	u16	s_max_mnt_count;	/* Maximal mount count */
	u16	s_magic;		/* Magic signature */
	u16	s_state;		/* File system state */
	u16	s_errors;		/* Behaviour when detecting errors */
	u16	s_minor_rev_level; 	/* minor revision level */
	u32	s_lastcheck;		/* time of last check */
	u32	s_checkinterval;	/* max. time between checks */
	u32	s_creator_os;		/* OS */
	u32	s_rev_level;		/* Revision level */
	u16	s_def_resuid;		/* Default uid for reserved blocks */
	u16	s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 * 
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	u32	s_first_ino; 		/* First non-reserved inode */
	u16 s_inode_size; 		/* size of inode structure */
	u16	s_block_group_nr; 	/* block group # of this superblock */
	u32	s_feature_compat; 	/* compatible feature set */
	u32	s_feature_incompat; 	/* incompatible feature set */
	u32	s_feature_ro_compat; 	/* readonly-compatible feature set */
	__u8	s_uuid[16];		/* 128-bit uuid for volume */
	char	s_volume_name[16]; 	/* volume name */
	char	s_last_mounted[64]; 	/* directory where last mounted */
	u32	s_algorithm_usage_bitmap; /* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the EXT2_COMPAT_PREALLOC flag is on.
	 */
	__u8	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
	__u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
	__u16	s_padding1;
	/*
	 * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	__u8	s_journal_uuid[16];	/* uuid of journal superblock */
	__u32	s_journal_inum;		/* inode number of journal file */
	__u32	s_journal_dev;		/* device number of journal file */
	__u32	s_last_orphan;		/* start of list of inodes to delete */
	__u32	s_hash_seed[4];		/* HTREE hash seed */
	__u8	s_def_hash_version;	/* Default hash version to use */
	__u8	s_reserved_char_pad;
	__u16	s_reserved_word_pad;
	u32	s_default_mount_opts;
    u32	s_first_meta_bg;        /* First metablock block group */
	__u32	s_reserved[190];	/* Padding to the end of the block */
};

struct ext2_inode_info {
    u32 i_data[15];

    struct inode vfs_inode;
};

struct ext2_sb_info {
    unsigned long s_sb_block;
    unsigned long s_inodes_per_group;   /* #Inodes in a group */
    unsigned long s_blocks_per_group;   /* Number of blocks in a group */
    unsigned long s_desc_per_block;     /* #group-desc per block */
    unsigned long s_groups_count;       /* Number of groups in the fs */
    int s_desc_per_block_bits;
    int s_inode_size;
    struct buffer_head ** s_group_desc;
};

/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc
{
    u32  bg_block_bitmap;       /* Blocks bitmap block */
    u32  bg_inode_bitmap;       /* Inodes bitmap block */
    u32  bg_inode_table;        /* Inodes table block */
    u16  bg_free_blocks_count;  /* Free blocks count */
    u16  bg_free_inodes_count;  /* Free inodes count */
    u16  bg_used_dirs_count;    /* Directories count */
    u16  bg_pad;
    u32  bg_reserved[3];
};

/*
 * Macro-instructions used to manage group descriptors
 */
#define EXT2_BLOCKS_PER_GROUP(s)    (EXT2_SB(s)->s_blocks_per_group)
#define EXT2_INODES_PER_GROUP(s)    (EXT2_SB(s)->s_inodes_per_group)
#define EXT2_DESC_PER_BLOCK(s)      (EXT2_SB(s)->s_desc_per_block)
#define EXT2_DESC_PER_BLOCK_BITS(s) (EXT2_SB(s)->s_desc_per_block_bits)

#define EXT2_INODE_SIZE(s)          (EXT2_SB(s)->s_inode_size)

#define EXT2_BLOCK_SIZE(s)      ((s)->s_blocksize)
#define EXT2_BLOCK_SIZE_BITS(s) ((s)->s_blocksize_bits)

struct inode *ext2_iget(struct super_block *sb, unsigned long ino);

static inline struct ext2_sb_info *EXT2_SB(struct super_block *sb)
{
    return sb->s_fs_info;
}

struct ext2_group_desc *
ext2_get_group_desc(struct super_block * sb,
                    unsigned int block_group,
                    struct buffer_head **bh);

#endif /* _EXT2_H_ */
