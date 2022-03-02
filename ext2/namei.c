// SPDX-License-Identifier: GPL-2.0

#include <errno.h>
#include <dcache.h>
#include <fs/ext2.h>

static struct dentry *
ext2_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    int res;
    ino_t ino;
    struct inode *inode;

    if (dentry->d_name.len > EXT2_NAME_LEN)
        return ERR_PTR(-ENAMETOOLONG);

    printk("%s: ===================== 1\n", __func__);
    res = ext2_inode_by_name(dir, &dentry->d_name, &ino);
    printk("%s: ===================== 2\n", __func__);
    if (res) {
        panic("lookup inode by name error!");
    } else {
        inode = ext2_iget(dir->i_sb, ino);
        if (inode == ERR_PTR(-ESTALE))
            panic("deleted inode referenced: %lu", (unsigned long) ino);
    }
    return d_splice_alias(inode, dentry);
}

const struct inode_operations ext2_dir_inode_operations = {
    .lookup = ext2_lookup,
};
