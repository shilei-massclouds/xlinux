/* SPDX-License-Identifier: GPL-2.0-only */

#include <module.h>

extern const struct kernel_symbol _start_ksymtab[];
extern const struct kernel_symbol _stop_ksymtab[];
extern const char _start_ksymtab_strings[];

#define ksymtab_num (_stop_ksymtab - _start_ksymtab)

void sbi_console_puts(const char *s);

struct module kernel_module;

void startup_init(void)
{
    int i;

    kernel_module.syms = _start_ksymtab;
    kernel_module.num_syms = ksymtab_num; 

    for (i = 0; i < kernel_module.num_syms; i++) {
        sbi_console_puts(_start_ksymtab[i].name);
        sbi_console_puts("\n");
    }
}
