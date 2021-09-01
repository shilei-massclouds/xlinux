/* SPDX-License-Identifier: GPL-2.0-only */

.balign 4
.section .text
.globl _start
    li a0, 'A'
    call sbi_console_putchar
