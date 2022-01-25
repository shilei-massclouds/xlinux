/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */

/* fs/open.c */
#define __NR_openat 56
__SYSCALL(__NR_openat, sys_openat)

/* mm/nommu.c, also with MMU */
#define __NR_brk 214
__SYSCALL(__NR_brk, sys_brk)
