/* SPDX-License-Identifier: GPL-2.0-only */

.balign 4
.section .text
.globl _start
_start:
    li sp, 0x80200000
    la a0, info
    call sbi_console_puts
    call startup_init

.balign 4
.section .data
info:
    .string "Hello! World!\n"
