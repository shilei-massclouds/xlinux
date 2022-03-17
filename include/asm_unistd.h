/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */

/* fs/open.c */
#define __NR_openat 56
__SYSCALL(__NR_openat, sys_openat)

/* fs/stat.c */
#define __NR_readlinkat 78
__SYSCALL(__NR_readlinkat, sys_readlinkat)

/* sys/sys.c */
#define __NR_uname 160
__SYSCALL(__NR_uname, sys_newuname)

/* mm/nommu.c, also with MMU */
#define __NR_brk 214
__SYSCALL(__NR_brk, sys_brk)
