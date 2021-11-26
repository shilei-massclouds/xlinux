/* SPDX-License-Identifier: GPL-2.0-only */

#include <mm.h>
#include <bug.h>
#include <vfs.h>
#include <path.h>
#include <errno.h>
#include <genhd.h>
#include <mount.h>
#include <namei.h>
#include <ramfs.h>
#include <export.h>
#include <kernel.h>
#include <params.h>
#include <printk.h>
#include <string.h>
#include <current.h>

extern char boot_command_line[];
extern bool ext2_initialized;

dev_t ROOT_DEV;

bool rootfs_initialized = false;
EXPORT_SYMBOL(rootfs_initialized);

char saved_root_name[64];
EXPORT_SYMBOL(saved_root_name);

static char *root_device_name;
int root_mountflags = MS_RDONLY | MS_SILENT;

static int
rootfs_init_fs_context(struct fs_context *fc)
{
    return ramfs_init_fs_context(fc);
}

struct file_system_type rootfs_fs_type = {
    .name = "rootfs",
    .init_fs_context = rootfs_init_fs_context,
};

static void
init_mount_tree(void)
{
    struct path root;
    struct vfsmount *mnt;

    mnt = vfs_kern_mount(&rootfs_fs_type, 0, "rootfs", NULL);
    if (IS_ERR(mnt))
        panic("Can't create rootfs!");

    root.mnt = mnt;
    root.dentry = mnt->mnt_root;

    set_fs_pwd(current->fs, &root);
    set_fs_root(current->fs, &root);
}

int
init_mkdir(const char *pathname, umode_t mode)
{
    struct path path;
    struct dentry *dentry;

    printk("### %s: mode(%o)\n", __func__, mode);
    dentry = kern_path_create(AT_FDCWD, pathname, &path,
                              LOOKUP_DIRECTORY);
    if (IS_ERR(dentry))
        return PTR_ERR(dentry);

    return vfs_mkdir(path.dentry->d_inode, dentry, mode);
}
EXPORT_SYMBOL(init_mkdir);

int
init_mknod(const char *filename, umode_t mode, unsigned int dev)
{
    struct path path;
    struct dentry *dentry;

    if (!(S_ISBLK(mode) || S_ISCHR(mode)))
        return -EINVAL;

    dentry = kern_path_create(AT_FDCWD, filename, &path, 0);
    if (IS_ERR(dentry))
        return PTR_ERR(dentry);

    return vfs_mknod(path.dentry->d_inode, dentry, mode, new_decode_dev(dev));
}
EXPORT_SYMBOL(init_mknod);

static int
root_dev_setup(char *param, char *value)
{
    strlcpy(saved_root_name, value, sizeof(saved_root_name));
    return 0;
}

dev_t
name_to_dev_t(const char *name)
{
    char *p;
    char s[32];

    name += 5;

    if (strlen(name) > 31)
        panic("bad name(%s)!");

    strcpy(s, name);
    for (p = s; *p; p++) {
        if (*p == '/')
            *p = '!';
    }

    return blk_lookup_devt(s, 0);
}

static void
get_fs_names(char *page)
{
    char *p, *next;
    char *s = page;
    int len = get_filesystem_list(page);

    page[len] = '\0';
    for (p = page-1; p; p = next) {
        next = strchr(++p, '\n');
        if (*p++ != '\t')
            continue;
        while ((*s++ = *p++) != '\n')
            ;
        s[-1] = '\0';
    }
    *s = '\0';
}

static int
do_mount_root(const char *name, const char *fs, const int flags)
{
    int ret;

    ret = init_mount(name, "/root", fs, flags);
    if (ret)
        panic("bad init mount /root");

    panic("%s: Todo:", __func__);
}

void
mount_block_root(char *name, int flags)
{
    char *p;
    struct page *page = alloc_page(GFP_KERNEL);
    char *fs_names = page_address(page);

    get_fs_names(fs_names);
    for (p = fs_names; *p; p += strlen(p)+1) {
        int err = do_mount_root(name, p, flags);
        switch (err) {
            case 0:
                return;
            case -EACCES:
            case -EINVAL:
                continue;
        }
        /*
         * Allow the user to distinguish between failed sys_open
         * and bad superblock on root device.
         * and give them a list of the available devices
         */
        printk("VFS: Cannot open root device \"%s\" or error %d\n",
               root_device_name, err);

        panic("VFS: Unable to mount root fs");
    }
    panic("%s: Todo: [%s]", __func__, fs_names);
}

void
mount_root(void)
{
    int err;
    err = create_dev("/dev/root", ROOT_DEV);
    if (err < 0)
        panic("Failed to create /dev/root: %d", err);

    mount_block_root("/dev/root", root_mountflags);
}

/*
 * Prepare the namespace - decide what/where to mount, load ramdisks, etc.
 */
void
prepare_namespace(void)
{
    root_device_name = saved_root_name;
    ROOT_DEV = name_to_dev_t(root_device_name);

    if (strncmp(root_device_name, "/dev/", 5) == 0)
        root_device_name += 5;

    printk("%s: ROOT_DEV(%x) name(%s)\n",
           __func__, ROOT_DEV, root_device_name);

    mount_root();
}

static struct kernel_param kernel_params[] = {
    { .name = "root", .setup_func = root_dev_setup, },
};

static unsigned int
num_kernel_params = sizeof(kernel_params) / sizeof(struct kernel_param);

static void
init_dirs(void)
{
    init_mkdir("dev", S_IFDIR | S_IRUGO | S_IWUSR | S_IXUGO);
    init_mkdir("root", S_IFDIR | S_IRWXU);
}

static int
init_module(void)
{
    printk("module[rootfs]: init begin ...\n");
    BUG_ON(!ext2_initialized);
    BUG_ON(parse_args(boot_command_line, kernel_params, num_kernel_params));
    init_mount_tree();
    rootfs_initialized = true;
    init_dirs();
    prepare_namespace();
    printk("module[rootfs]: init end!\n");
    return 0;
}
