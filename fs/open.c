// SPDX-License-Identifier: GPL-2.0-only

#include <fs.h>
#include <stat.h>
#include <errno.h>
#include <fcntl.h>
#include <dcache.h>
#include <export.h>
#include <syscalls.h>

static int do_dentry_open(struct file *f,
                          struct inode *inode,
                          int (*open)(struct inode *, struct file *))
{
    int error;

    f->f_inode = inode;
    f->f_mapping = inode->i_mapping;

    f->f_op = inode->i_fop;
    BUG_ON(!f->f_op);

    /* POSIX.1-2008/SUSv4 Section XSI 2.9.7 */
    if (S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode))
        f->f_mode |= FMODE_ATOMIC_POS;

    /* normally all 3 are set; ->open() can clear them if needed */
    f->f_mode |= FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE;
    printk("%s 1 f_mode(%x)\n", __func__, f->f_mode);
    if (!open)
        open = f->f_op->open;
    if (open) {
        error = open(inode, f);
        if (error)
            panic("open error!");
    }
    f->f_mode |= FMODE_OPENED;
    if ((f->f_mode & FMODE_READ) &&
        likely(f->f_op->read || f->f_op->read_iter))
        f->f_mode |= FMODE_CAN_READ;

    f->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);
    file_ra_state_init(&f->f_ra, f->f_mapping->host->i_mapping);
    return 0;
}

/**
 * vfs_open - open the file at the given path
 * @path: path to open
 * @file: newly allocated file with f_flag initialized
 * @cred: credentials to use
 */
int vfs_open(const struct path *path, struct file *file)
{
    file->f_path = *path;
    return do_dentry_open(file, d_backing_inode(path->dentry), NULL);
}

/*
 * Called when an inode is about to be open.
 * We use this to disallow opening large files on 32bit systems if
 * the caller didn't specify O_LARGEFILE.  On 64bit systems we force
 * on this flag in sys_open.
 */
int generic_file_open(struct inode *inode, struct file *filp)
{
    if (!(filp->f_flags & O_LARGEFILE) &&
        i_size_read(inode) > MAX_NON_LFS)
        return -EOVERFLOW;
    return 0;
}
EXPORT_SYMBOL(generic_file_open);

long
_do_sys_open(int dfd, const char *filename, int flags, umode_t mode)
{
    panic("%s: filename(%s) flags(%x) mode(%x)!", __func__, filename, flags, mode);
}

void init_open(void)
{
    do_sys_open = _do_sys_open;
}
