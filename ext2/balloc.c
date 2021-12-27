// SPDX-License-Identifier: GPL-2.0

#include <printk.h>
#include <fs/ext2.h>

struct ext2_group_desc *
ext2_get_group_desc(struct super_block *sb,
                    unsigned int block_group,
                    struct buffer_head **bh)
{
    unsigned long offset;
    unsigned long group_desc;
    struct ext2_group_desc *desc;
    struct ext2_sb_info *sbi = EXT2_SB(sb);

    printk("block_group = %d, groups_count = %lu\n",
           block_group, sbi->s_groups_count);

    if (block_group >= sbi->s_groups_count)
        panic("block_group = %d, groups_count = %lu",
              block_group, sbi->s_groups_count);

    group_desc = block_group >> EXT2_DESC_PER_BLOCK_BITS(sb);
    offset = block_group & (EXT2_DESC_PER_BLOCK(sb) - 1);
    if (!sbi->s_group_desc[group_desc])
        panic("block_group = %d, group_desc = %lu, desc = %lu",
              block_group, group_desc, offset);

    desc = (struct ext2_group_desc *) sbi->s_group_desc[group_desc]->b_data;
    if (bh)
        *bh = sbi->s_group_desc[group_desc];

    return desc + offset;
}
