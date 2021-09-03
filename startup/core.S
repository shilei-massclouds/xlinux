/* SPDX-License-Identifier: GPL-2.0-only */

#include <page.h>

.balign 4
.section .text
.globl _start
_start:

.option push
.option norelax
    la gp, __global_pointer$
.option pop

    # Init stack for C function
    la sp, init_stack_top

    la a0, info
    call sbi_console_puts

    call setup_trampoline

    call startup_init

.balign 4
.section .data
info:
    .string "Hello! World!\n"
