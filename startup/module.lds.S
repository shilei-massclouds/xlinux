/* SPDX-License-Identifier: GPL-2.0-only */

#include <page.h>

#define LOAD_OFFSET 0x80200000

OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    . = LOAD_OFFSET;

    _start = .;

    .text : AT(ADDR(.text) - LOAD_OFFSET) {
        *(.text);
    }

    .rodata : AT(ADDR(.rodata) - LOAD_OFFSET) {
        *(.rodata);
    }

    _ksymtab : AT(ADDR(_ksymtab) - LOAD_OFFSET) {
        _start_ksymtab = .;
        KEEP(*(SORT(_ksymtab)));
        _stop_ksymtab = .;
    }

    _ksymtab_strings : AT(ADDR(_ksymtab_strings) - LOAD_OFFSET) {
        KEEP(*(_ksymtab_strings));
    }

    .data : AT(ADDR(.data) - LOAD_OFFSET) {
        . = ALIGN(PAGE_SIZE);
        __start_init_task = .;
        init_thread_union = .;
        . = __start_init_task + 4*PAGE_SIZE;
        __end_init_task = .;
        *(.data);
        __global_pointer$ = . + 0x800;
    }

    _bss_start = .;
    .bss : AT(ADDR(.bss) - LOAD_OFFSET) {
        *(.bss);
    }
    _bss_stop = .;

    _end = .;
}
