// SPDX-License-Identifier: GPL-2.0

#include <fs.h>
#include <export.h>

static int chrdev_open(struct inode *inode, struct file *filp)
{
    panic("no char device open!");
}

/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */
const struct file_operations def_chr_fops = {
    .open = chrdev_open,
};
EXPORT_SYMBOL(def_chr_fops);
