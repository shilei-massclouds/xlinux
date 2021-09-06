/* SPDX-License-Identifier: GPL-2.0-only */

#include <types.h>
#include <module.h>
#include <sbi.h>
#include <image.h>

#define FLASH_START 0x20000000UL

extern char _start[];

extern const struct kernel_symbol _start_ksymtab[];
extern const struct kernel_symbol _stop_ksymtab[];
extern const char _start_ksymtab_strings[];

#define ksymtab_num (_stop_ksymtab - _start_ksymtab)

struct module kernel_module;

static void init_kernel_module(void)
{
    kernel_module.syms = _start_ksymtab;
    kernel_module.num_syms = ksymtab_num;
}

static void load_modules(void)
{
    struct image_header *header = (struct image_header *)(&_start);

    {
        char tmp[64] = {0};
        hex_to_str(header->res2, tmp, sizeof(tmp));
        sbi_console_puts(tmp);
        sbi_console_puts("\n");
    }
}

void startup_init(void)
{
    sbi_console_puts("init ... \n");

    init_kernel_module();

    load_modules();
    /*
    int i;
    for (i = 0; i < kernel_module.num_syms; i++) {
        char tmp[64];
        sbi_console_puts(_start_ksymtab[i].name);
        sbi_console_puts("\n");

        hex_to_str(_start_ksymtab[i].value, tmp, sizeof(tmp));
        sbi_console_puts(tmp);
        sbi_console_puts("\n");
    }
    */
}
