/* SPDX-License-Identifier: GPL-2.0-only */

#include <page.h>
#include <csr.h>

#define RISCV_SZPTR 8

#define RISCV_HEADER_VERSION    2
#define RISCV_IMAGE_MAGIC       "RISCV\0\0\0"
#define RISCV_IMAGE_MAGIC2      "RSC\x05"

.section .head.text, "ax"
.globl _start
_start:
    j _start_kernel                 /* code0 */
    .word  0                        /* code1 */
    .dword 0x200000                 /* text_offset: Image load offset */
    .dword _end - _start            /* image_size: Effective image size */
    .dword 0                        /* flags: kernel flags */
    .word  RISCV_HEADER_VERSION     /* version */
    .word  0                        /* res1 */
    .dword _data_end - _start       /* res2: Image file size on disk */
    .ascii RISCV_IMAGE_MAGIC        /* magic */
    .balign 4
    .ascii RISCV_IMAGE_MAGIC2       /* magic2 */
    .word  0                        /* res3 */

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

    # Prepare for paging
    la a0, early_pgd
    srl a0, a0, PAGE_SHIFT
    li a1, SATP_MODE
    or a0, a0, a1
    sfence.vma
    csrw CSR_SATP, a0

    # Switch point for paging
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

.section .init.text, "ax"
_start_kernel:
    mv s0, a0
    mv s1, a1

.option push
.option norelax
    la gp, __global_pointer$
.option pop

    /* Clear BSS for flat non-ELF images */
    la a3, _bss_start
    la a4, _bss_stop
    ble a4, a3, clear_bss_done
clear_bss:
    sd zero, (a3)
    add a3, a3, RISCV_SZPTR
    blt a3, a4, clear_bss
clear_bss_done:

    # Init stack for C function
    la sp, init_stack_top

    la a0, info
    call sbi_puts

    # Init pgd
    mv a0, s1
    call setup_fixmap_pge
    call setup_early_pge
    call setup_flash_pge

    # Enable paging
    call relocate

    # Restore stack for C function
    la sp, init_stack_top

    call load_modules

.balign 4
.section .data, "aw"
info:
    .string "Hello! World!\n"
