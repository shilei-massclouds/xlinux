/* SPDX-License-Identifier: GPL-2.0-only */

#include <types.h>
#include <module.h>

extern const struct kernel_symbol _start_ksymtab[];
extern const struct kernel_symbol _stop_ksymtab[];
extern const char _start_ksymtab_strings[];

#define ksymtab_num (_stop_ksymtab - _start_ksymtab)

void sbi_console_puts(const char *s);
void hex_to_str(unsigned long num, char *str, size_t size);

struct module kernel_module;

void startup_init(void)
{
    int i;

    kernel_module.syms = _start_ksymtab;
    kernel_module.num_syms = ksymtab_num;

    for (i = 0; i < kernel_module.num_syms; i++) {
        char tmp[64];
        sbi_console_puts(_start_ksymtab[i].name);
        sbi_console_puts("\n");

        hex_to_str(_start_ksymtab[i].value, tmp, sizeof(tmp));
        sbi_console_puts(tmp);
        sbi_console_puts("\n");
    }
}
