// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <printk.h>
#include <string.h>
#include <uaccess.h>
#include <utsname.h>
#include <syscalls.h>

long _do_sys_newuname(struct new_utsname *name)
{
    struct new_utsname tmp;

    memcpy(&tmp, utsname(), sizeof(tmp));
    if (copy_to_user(name, &tmp, sizeof(tmp)))
        return -EFAULT;
    return 0;
}

static int
init_module(void)
{
    printk("module[sys]: init begin ...\n");

    do_sys_newuname = _do_sys_newuname;

    printk("module[sys]: init end!\n");
    return 0;
}
