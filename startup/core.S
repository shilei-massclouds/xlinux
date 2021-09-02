/* SPDX-License-Identifier: GPL-2.0-only */

#include <page.h>

.balign 4
.section .text
.globl _start
_start:
#la sp, __end_init_task
    li sp, 0x80200000
    la a0, info
    la gp, __global_pointer$
    call sbi_console_puts
    call startup_init

.balign 4
.section .data
info:
    .string "Hello! World!\n"
