// SPDX-License-Identifier: GPL-2.0

#include <namei.h>
#include <export.h>
#include <current.h>

int init_chdir(const char *filename)
{
    struct path path;
    int error;

    error = kern_path(filename, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &path);
    if (error)
        return error;

    set_fs_pwd(current->fs, &path);
    path_put(&path);
    return error;
}
EXPORT_SYMBOL(init_chdir);

int init_chroot(const char *filename)
{
    int error;
    struct path path;

    error = kern_path(filename, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &path);
    if (error)
        return error;

    set_fs_root(current->fs, &path);
    path_put(&path);
    return error;
}
EXPORT_SYMBOL(init_chroot);
