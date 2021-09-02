/* SPDX-License-Identifier: GPL-2.0-only */

OUTPUT_ARCH(riscv)
ENTRY(_start)

#define PAGE_OFFSET  0x80200000

SECTIONS
{
    . = PAGE_OFFSET;

    _start = .;

    .text : AT(ADDR(.text) - PAGE_OFFSET) {
        *(.text);
    }

    _ksymtab : AT(ADDR(_ksymtab) - PAGE_OFFSET) {
        _start_ksymtab = .;
        KEEP(*(SORT(_ksymtab)));
        _stop_ksymtab = .;
    }

    _ksymtab_strings : AT(ADDR(_ksymtab_strings) - PAGE_OFFSET) {
        KEEP(*(_ksymtab_strings));
    }

    .data : AT(ADDR(.data) - PAGE_OFFSET) {
        *(.data);
    }

    _bss_start = .;
    .bss : AT(ADDR(.bss) - PAGE_OFFSET) {
        *(.bss);
    }
    _bss_stop = .;

    _end = .;
}
