// SPDX-License-Identifier: GPL-2.0-only

#include <export.h>
#include <syscalls.h>

do_sys_open_t do_sys_open;
EXPORT_SYMBOL(do_sys_open);

SYSCALL_DEFINE4(openat, int, dfd, const char *, filename, int, flags,
                umode_t, mode)
{
    return do_sys_open(dfd, filename, flags, mode);
}

do_sys_brk_t do_sys_brk;
EXPORT_SYMBOL(do_sys_brk);

SYSCALL_DEFINE1(brk, unsigned long, brk)
{
    return do_sys_brk(brk);
}
