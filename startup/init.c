/* SPDX-License-Identifier: GPL-2.0-only */

#include <types.h>
#include <module.h>
#include <sbi.h>
#include <image.h>
#include <fixmap.h>
#include <elf.h>

/* n must be power of 2 */
#define ROUND_UP(x, n) (((x) + (n) - 1ul) & ~((n) - 1ul))

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

static uintptr_t modules_base(void)
{
    uintptr_t base = (FLASH_VA + FLASH_HEAD_SIZE);
    struct image_header *hdr = (struct image_header *) base;

    return ROUND_UP((base + hdr->res2), 8);
}

static void load_modules(uintptr_t base)
{
    Elf64_Ehdr *header = (Elf64_Ehdr *) base;

    {
        char tmp[64] = {0};
        sbi_console_puts(header->e_ident);
        sbi_console_puts("\n");
        hex_to_str(header->e_shoff, tmp, sizeof(tmp));
        sbi_console_puts(tmp);
        sbi_console_puts("\n");
    }
}

void startup_init(void)
{
    sbi_console_puts("init ... \n");

    init_kernel_module();

    load_modules(modules_base());

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
