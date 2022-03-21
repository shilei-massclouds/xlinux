// SPDX-License-Identifier: GPL-2.0-only

#include <fs.h>
#include <file.h>
#include <stat.h>
#include <errno.h>
#include <namei.h>
#include <fcntl.h>
#include <dcache.h>
#include <export.h>
#include <fdtable.h>
#include <openat2.h>
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

static struct open_how build_open_how(int flags, umode_t mode)
{
    struct open_how how = {
        .flags = flags,
        .mode = mode,
    };

    return how;
}

struct filename *
getname(const char *filename)
{
    return getname_flags(filename, 0, NULL);
}

static inline void __clear_open_fd(unsigned int fd, struct fdtable *fdt)
{
    __clear_bit(fd, fdt->open_fds);
    __clear_bit(fd / BITS_PER_LONG, fdt->full_fds_bits);
}

static void __put_unused_fd(struct files_struct *files, unsigned int fd)
{
    struct fdtable *fdt = files_fdtable(files);
    __clear_open_fd(fd, fdt);
    if (fd < files->next_fd)
        files->next_fd = fd;
}

void put_unused_fd(unsigned int fd)
{
    struct files_struct *files = current->files;
    __put_unused_fd(files, fd);
}
EXPORT_SYMBOL(put_unused_fd);

static long
do_sys_openat2(int dfd, const char *filename, struct open_how *how)
{
    int fd;
    struct open_flags op;
    struct filename *tmp;

    tmp = getname(filename);
    if (IS_ERR(tmp))
        return PTR_ERR(tmp);

    fd = get_unused_fd_flags(how->flags);
    if (fd >= 0) {
        struct file *f = do_filp_open(dfd, tmp, &op);
        if (IS_ERR(f)) {
            put_unused_fd(fd);
            fd = PTR_ERR(f);
        } else {
            fd_install(fd, f);
        }
    }

    printk("%s: fd(%d) filename(%s) flags(%x) mode(%x)!\n",
          __func__, fd, filename, how->flags, how->mode);
    return fd;
}

long
_do_sys_open(int dfd, const char *filename, int flags, umode_t mode)
{
    struct open_how how = build_open_how(flags, mode);
    return do_sys_openat2(dfd, filename, &how);
}

void init_open(void)
{
    do_sys_open = _do_sys_open;
}
