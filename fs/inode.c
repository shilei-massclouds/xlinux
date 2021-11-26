// SPDX-License-Identifier: GPL-2.0-only

#include <fs.h>
#include <bug.h>
#include <export.h>
#include <string.h>

void
inode_init_once(struct inode *inode)
{
    memset(inode, 0, sizeof(*inode));
    /*
    INIT_HLIST_NODE(&inode->i_hash);
    INIT_LIST_HEAD(&inode->i_devices);
    INIT_LIST_HEAD(&inode->i_io_list);
    INIT_LIST_HEAD(&inode->i_wb_list);
    INIT_LIST_HEAD(&inode->i_lru);
    */
    //__address_space_init_once(&inode->i_data);
    //i_size_ordered_init(inode);
}
EXPORT_SYMBOL(inode_init_once);

struct inode *
iget5_locked(struct super_block *sb, unsigned long hashval,
             int (*test)(struct inode *, void *),
             int (*set)(struct inode *, void *), void *data)
{
    panic("%s: ", __func__);
    /*
    struct inode *inode = ilookup5(sb, hashval, test, data);

    if (!inode) {
        struct inode *new = alloc_inode(sb);

        if (new) {
            new->i_state = 0;
            inode = inode_insert5(new, hashval, test, set, data);
            if (unlikely(inode != new))
                destroy_inode(new);
        }
    }
    return inode;
    */
}
EXPORT_SYMBOL(iget5_locked);
