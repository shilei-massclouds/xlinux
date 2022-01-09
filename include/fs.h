/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_FS_H
#define _LINUX_FS_H

#include <cred.h>
#include <list.h>
#include <page.h>
#include <path.h>
#include <fcntl.h>
#include <types.h>
#include <xarray.h>

#define BLOCK_SIZE_BITS 10
#define BLOCK_SIZE      (1<<BLOCK_SIZE_BITS)

#define SB_ACTIVE   (1<<30)

/*
 * sb->s_flags.  Note that these mirror the equivalent MS_* flags where
 * represented in both.
 */
#define SB_RDONLY    1  /* Mount read-only */
#define SB_NOSUID    2  /* Ignore suid and sgid bits */
#define SB_NODEV     4  /* Disallow access to device special files */
#define SB_NOEXEC    8  /* Disallow program execution */
#define SB_SYNCHRONOUS  16  /* Writes are synced at once */
#define SB_MANDLOCK 64  /* Allow mandatory locks on an FS */
#define SB_DIRSYNC  128 /* Directory modifications are synchronous */
#define SB_NOATIME  1024    /* Do not update access times. */
#define SB_NODIRATIME   2048    /* Do not update directory access times */
#define SB_SILENT   32768
#define SB_POSIXACL (1<<16) /* VFS does not apply the umask */
#define SB_INLINECRYPT  (1<<17) /* Use blk-crypto for encrypted files */
#define SB_KERNMOUNT    (1<<22) /* this is a kern_mount call */
#define SB_I_VERSION    (1<<23) /* Update inode I_version field */
#define SB_LAZYTIME (1<<25) /* Update the on-disk [acm]times lazily */

#define SB_SUBMOUNT (1<<26)
#define SB_NOSEC    (1<<28)
#define SB_NOUSER   (1<<31)

struct super_block;
struct buffer_head;

/*
 * flags in file.f_mode.  Note that FMODE_READ and FMODE_WRITE must correspond
 * to O_WRONLY and O_RDWR via the strange trick in do_dentry_open()
 */

/* file is open for reading */
#define FMODE_READ      ((__force fmode_t)0x1)
/* file is open for writing */
#define FMODE_WRITE     ((__force fmode_t)0x2)
/* File is opened for execution with sys_execve / sys_uselib */
#define FMODE_EXEC      ((__force fmode_t)0x20)
/* File is opened with O_EXCL (only set for block devices) */
#define FMODE_EXCL      ((__force fmode_t)0x80)
/* File was opened by fanotify and shouldn't generate fanotify events */
#define FMODE_NONOTIFY  ((__force fmode_t)0x4000000)

#define __FMODE_NONOTIFY    ((__force int) FMODE_NONOTIFY)

#define OPEN_FMODE(flag) \
    ((fmode_t)(((flag + 1) & O_ACCMODE) | (flag & __FMODE_NONOTIFY)))

#define __FMODE_EXEC    ((__force int) FMODE_EXEC)

#define MAY_EXEC        0x00000001
#define MAY_WRITE       0x00000002
#define MAY_READ        0x00000004
#define MAY_APPEND      0x00000008
#define MAY_ACCESS      0x00000010
#define MAY_OPEN        0x00000020
#define MAY_CHDIR       0x00000040

/*
 * Inode state bits.  Protected by inode->i_lock
 */
#define __I_NEW         3
#define I_NEW           (1 << __I_NEW)
#define I_CREATING      (1 << 15)

struct file;

struct address_space_operations {
    int (*readpage)(struct file *, struct page *);
};

struct address_space {
    struct inode    *host;
    struct xarray   i_pages;
    unsigned long   nrpages;
    gfp_t           gfp_mask;

    const struct address_space_operations *a_ops;
} __attribute__((aligned(sizeof(long))));

/*
 * Write life time hint values.
 * Stored in struct inode as u8.
 */
enum rw_hint {
    WRITE_LIFE_NOT_SET  = 0,
};

struct filename {
    const char *name;   /* pointer to actual string */
    const char iname[];
};

extern struct kmem_cache *names_cachep;

#define __getname() kmem_cache_alloc(names_cachep, GFP_KERNEL)

struct fs_context;

struct fs_context_operations {
    int (*get_tree)(struct fs_context *fc);
};

enum fs_context_purpose {
    FS_CONTEXT_FOR_MOUNT,       /* New superblock for explicit mount */
    FS_CONTEXT_FOR_SUBMOUNT,    /* New superblock for automatic submount */
    FS_CONTEXT_FOR_RECONFIGURE, /* Superblock reconfiguration (remount) */
};

struct fs_context {
    const struct fs_context_operations *ops;
    struct file_system_type *fs_type;
    void *fs_private;           /* The filesystem's context */
    struct dentry *root;        /* The root and superblock */
    void *s_fs_info;            /* Proposed s_fs_info */
    unsigned int sb_flags;      /* Proposed superblock flags (SB_*) */
    unsigned int sb_flags_mask; /* Superblock flags that were changed */
    const char *source;         /* The source name (eg. dev path) */
    enum fs_context_purpose purpose:8;
};

struct super_operations {
    struct inode *(*alloc_inode)(struct super_block *sb);
};

struct super_block {
    struct list_head s_list;    /* Keep this first */
    dev_t s_dev;                /* search index; _not_ kdev_t */
    struct dentry *s_root;
    unsigned long s_flags;
    void *s_fs_info;            /* Proposed s_fs_info */
    const struct super_operations *s_op;
    struct list_head s_inodes;  /* all inodes */
    struct hlist_node s_instances;
    struct block_device *s_bdev;
    struct file_system_type *s_type;
    char s_id[32];              /* Informational name */
    fmode_t s_mode;
    unsigned char s_blocksize_bits;
    unsigned long s_blocksize;
};

struct inode_operations {
    struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);
    int (*mkdir) (struct inode *,struct dentry *,umode_t);
    int (*mknod) (struct inode *,struct dentry *,umode_t,dev_t);
};

struct inode {
    umode_t             i_mode;

    /* Stat data, not accessed from path walking */
    unsigned long       i_ino;

    loff_t              i_size;
    dev_t               i_rdev;

    u8                  i_blkbits;
    blkcnt_t            i_blocks;

    struct hlist_node   i_hash;

    const struct inode_operations *i_op;
    const struct file_operations  *i_fop;

    struct super_block *i_sb;
    struct address_space *i_mapping;
    struct address_space i_data;

    unsigned long       i_state;
    struct list_head    i_sb_list;

    struct block_device *i_bdev;
};

struct pseudo_fs_context {
    const struct super_operations *ops;
    unsigned long magic;
};

struct file_operations {
};

struct file_system_type {
    const char *name;

    int fs_flags;
#define FS_REQUIRES_DEV     1
#define FS_BINARY_MOUNTDATA 2
#define FS_HAS_SUBTYPE      4
#define FS_USERNS_MOUNT     8   /* Can be mounted by userns root */
#define FS_DISALLOW_NOTIFY_PERM 16  /* Disable fanotify permission events */
#define FS_RENAME_DOES_D_MOVE   32768 /* FS will handle d_move() during rename() internally. */

    int (*init_fs_context)(struct fs_context *);

    struct dentry *(*mount)(struct file_system_type *,
                            int, const char *);

    struct file_system_type *next;
    struct hlist_head fs_supers;
};

struct fs_struct {
    struct path root;
    struct path pwd;
};

/*
 * open.c
 */
struct open_flags {
    int open_flag;
    umode_t mode;
    int acc_mode;
    int intent;
    int lookup_flags;
};

struct file {
    unsigned int    f_flags;
    fmode_t         f_mode;
};

extern bool rootfs_initialized;

struct fs_context *
fs_context_for_mount(struct file_system_type *fs_type,
                     unsigned int sb_flags);

void
put_fs_context(struct fs_context *fc);

void
set_fs_root(struct fs_struct *fs, const struct path *path);

void
set_fs_pwd(struct fs_struct *fs, const struct path *path);

static inline void
get_fs_root(struct fs_struct *fs, struct path *root)
{
    *root = fs->root;
}

static inline struct file_system_type *
get_filesystem(struct file_system_type *fs)
{
    return fs;
}

int
get_tree_nodev(struct fs_context *fc,
               int (*fill_super)(struct super_block *sb,
                                 struct fs_context *fc));

struct inode *
new_inode(struct super_block *sb);

static inline void
iput(struct inode *inode)
{
}

struct dentry *
simple_lookup(struct inode *dir,
              struct dentry *dentry,
              unsigned int flags);

int
init_mkdir(const char *pathname, umode_t mode);

int
get_filesystem_list(char *buf);

int
register_filesystem(struct file_system_type * fs);

struct file_system_type *
get_fs_type(const char *name);

struct dentry *
mount_bdev(struct file_system_type *fs_type,
           int flags, const char *dev_name,
           int (*fill_super)(struct super_block *, void *, int));

struct block_device *
blkdev_get_by_path(const char *path, fmode_t mode, void *holder);

struct inode *
iget_locked(struct super_block *sb, unsigned long ino);

struct inode *
iget5_locked(struct super_block *sb, unsigned long hashval,
             int (*test)(struct inode *, void *),
             int (*set)(struct inode *, void *), void *data);

void
bdev_cache_init(void);

struct pseudo_fs_context *
init_pseudo(struct fs_context *fc, unsigned long magic);

void inode_init_once(struct inode *inode);

void inode_init(void);

static inline int inode_unhashed(struct inode *inode)
{
    return hlist_unhashed(&inode->i_hash);
}

static inline void
i_size_write(struct inode *inode, loff_t i_size)
{
    inode->i_size = i_size;
}

struct super_block *
sget(struct file_system_type *type,
     int (*test)(struct super_block *,void *),
     int (*set)(struct super_block *,void *),
     int flags,
     void *data);

int sb_set_blocksize(struct super_block *sb, int size);

int sb_min_blocksize(struct super_block *sb, int size);

static inline loff_t i_size_read(const struct inode *inode)
{
    return inode->i_size;
}

int init_chdir(const char *filename);

int init_chroot(const char *filename);

int kernel_execve(const char *kernel_filename,
                  const char *const *argv, const char *const *envp);

struct file *do_filp_open(int dfd, struct filename *pathname,
                          const struct open_flags *op);

struct file *alloc_empty_file(int flags, const struct cred *cred);

struct mount *__lookup_mnt(struct vfsmount *mnt, struct dentry *dentry);

typedef int (get_block_t)(struct inode *inode, sector_t iblock,
                          struct buffer_head *bh_result, int create);

int mpage_readpage(struct page *page, get_block_t get_block);

int bdev_read_page(struct block_device *bdev, sector_t sector,
                   struct page *page);

#endif /* _LINUX_FS_H */
