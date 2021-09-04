/* SPDX-License-Identifier: GPL-2.0-only */

#include <page.h>

.section .head.text, "ax"
.globl _start
_start:
    j _start_kernel

.section .text, "ax"
_start_kernel:

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
.section .data, "aw"
info:
    .string "Hello! World!\n"
