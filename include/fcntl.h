/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _ASM_GENERIC_FCNTL_H
#define _ASM_GENERIC_FCNTL_H

#define O_ACCMODE   00000003
#define O_RDONLY    00000000
#define O_WRONLY    00000001
#define O_RDWR      00000002
#define O_CREAT     00000100    /* not fcntl */
#define O_LARGEFILE 00100000

#define O_PATH      010000000
#define __O_TMPFILE 020000000

#endif /* _ASM_GENERIC_FCNTL_H */
