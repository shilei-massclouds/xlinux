/* SPDX-License-Identifier: GPL-2.0-only */

#include <thread_info.h>

#define LOAD_OFFSET PAGE_OFFSET

OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    . = LOAD_OFFSET;

    _start = .;

    .head.text : AT(ADDR(.head.text) - LOAD_OFFSET) {
        *(.head.text);
    }

    . = ALIGN(PAGE_SIZE);

    .init.text : AT(ADDR(.init.text) - LOAD_OFFSET) {
        *(.init.text);
    }

    .init.data : AT(ADDR(.init.data) - LOAD_OFFSET) {
        *(.init.data);
    }

    .text : AT(ADDR(.text) - LOAD_OFFSET) {
        *(.text);
    }

    .rodata : AT(ADDR(.rodata) - LOAD_OFFSET) {
        *(.rodata);
    }

    _ksymtab : AT(ADDR(_ksymtab) - LOAD_OFFSET) {
        _start_ksymtab = .;
        KEEP(*(SORT(_ksymtab)));
        _end_ksymtab = .;
    }

    _ksymtab_strings : AT(ADDR(_ksymtab_strings) - LOAD_OFFSET) {
        KEEP(*(_ksymtab_strings));
    }

    _data_start = .;
    .data : AT(ADDR(.data) - LOAD_OFFSET) {
        . = ALIGN(PAGE_SIZE);
        init_stack = .;
        . = init_stack + THREAD_SIZE;
        init_stack_top = .;
        *(.data);
        __global_pointer$ = . + 0x800;
    }
    _data_end = .;

    . = ALIGN(PAGE_SIZE);

    _bss_start = .;
    .bss : AT(ADDR(.bss) - LOAD_OFFSET) {
        . = ALIGN(PAGE_SIZE);
        *(.bss..page_aligned);
        . = ALIGN(PAGE_SIZE);
        *(.bss);
    }
    _bss_stop = .;

    _end = .;
}
