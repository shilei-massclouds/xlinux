/* SPDX-License-Identifier: GPL-2.0-only */

#include <page.h>
#include <csr.h>

.section .head.text, "ax"
.globl _start
_start:
    j _start_kernel

.align 2
relocate:
    # Distance between va and pa
    li a1, PAGE_OFFSET
    la a2, _start
    sub a1, a1, a2

    # Relocate return address of this function itself
    add ra, ra, a1

    # Point stvec to correspoinding va of that exact position
    la a2, 1f
    add a2, a2, a1
    csrw CSR_STVEC, a2

    la a0, trampoline_pgd
    srl a0, a0, PAGE_SHIFT
    li a1, SATP_MODE
    or a0, a0, a1
    sfence.vma
    csrw CSR_SATP, a0
.align 2
1:
    la a2, .Lforever_park
    csrw CSR_STVEC, a2

.option push
.option norelax
    la gp, __global_pointer$
.option pop

    ret

.Lforever_park:
    wfi
    j .Lforever_park

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

    call relocate

    # Restore stack for C function
    la sp, init_stack_top

    call startup_init

.balign 4
.section .data, "aw"
info:
    .string "Hello! World!\n"
