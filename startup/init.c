/* SPDX-License-Identifier: GPL-2.0-only */

#include <types.h>
#include <module.h>
#include <sbi.h>
#include <image.h>
#include <fixmap.h>
#include <elf.h>
#include <kernel.h>

/* n must be power of 2 */
#define ROUND_UP(x, n) (((x) + (n) - 1ul) & ~((n) - 1ul))

extern char _end[];

extern const struct kernel_symbol _start_ksymtab[];
extern const struct kernel_symbol _stop_ksymtab[];
extern const char _start_ksymtab_strings[];

#define ksymtab_num (_stop_ksymtab - _start_ksymtab)

struct module kernel_module;

struct layout {
    unsigned int size;
    unsigned int text_size;
    unsigned int ro_size;
};

struct load_info {
    const char *name;
    unsigned long len;
    Elf64_Ehdr *hdr;
    Elf64_Shdr *sechdrs;
    char *secstrings;
    char *strtab;

    struct {
        unsigned int sym;
        unsigned int str;
    } index;

    struct layout layout;
};

static void init_kernel_module(void)
{
    kernel_module.syms = _start_ksymtab;
    kernel_module.num_syms = ksymtab_num;
}

static long
get_offset(unsigned int *size, Elf64_Shdr *s)
{
    long ret;

    ret = ALIGN(*size, s->sh_addralign ? : 1);
    *size = ret + s->sh_size;
    return ret;
}

static void
layout_sections(struct load_info *info)
{
    static unsigned long const masks[][2] = {
        { SHF_EXECINSTR | SHF_ALLOC, 0 },
        { SHF_ALLOC, SHF_WRITE },
        { SHF_WRITE | SHF_ALLOC, 0 },
        { SHF_ALLOC, 0 }
    };
    unsigned int m, i;

    for (i = 0; i < info->hdr->e_shnum; i++)
        info->sechdrs[i].sh_entsize = ~0UL;

    for (m = 0; m < ARRAY_SIZE(masks); ++m) {
        for (i = 0; i < info->hdr->e_shnum; ++i) {
            Elf64_Shdr *s = info->sechdrs + i;
            const char *sname = info->secstrings + s->sh_name;

            if ((s->sh_flags & masks[m][0]) != masks[m][0]
                || (s->sh_flags & masks[m][1])
                || s->sh_entsize != ~0UL)
                continue;

            s->sh_entsize = get_offset(&(info->layout.size), s);
        }

        switch (m) {
        case 0: /* executable */
            info->layout.text_size = info->layout.size;
            break;
        case 1: /* RO: text and ro-data */
            info->layout.ro_size = info->layout.size;
            break;
        }
    }
}

static void
rewrite_section_headers(struct load_info *info)
{
    int i;

    /* Skip 0 because it is NULL segment. */
    for (i = 1; i < info->hdr->e_shnum; i++) {
        Elf64_Shdr *s = info->sechdrs + i;
        s->sh_addr = (size_t)info->hdr + s->sh_offset;
        /*
        char tmp[64] = {0};
        //hex_to_str(shdr->sh_offset, tmp, sizeof(tmp));
        hex_to_str(shdr->sh_flags, tmp, sizeof(tmp));
        sbi_console_puts(tmp);
        sbi_console_puts("\n");
        */
    }
}

static void
setup_load_info(uintptr_t base, struct load_info *info)
{
    int i;

    info->name = NULL;
    info->hdr = (Elf64_Ehdr *)base;
    info->sechdrs = (void *)info->hdr + info->hdr->e_shoff;
    info->secstrings = (void *)info->hdr +
        info->sechdrs[info->hdr->e_shstrndx].sh_offset;
    info->strtab = NULL;

    for (i = 1; i < info->hdr->e_shnum; i++) {
        if (info->sechdrs[i].sh_type == SHT_SYMTAB) {
            info->index.sym = i;
            info->index.str = info->sechdrs[i].sh_link;
            info->strtab = (char *)info->hdr
                + info->sechdrs[info->index.str].sh_offset;
            break;
        }
    }
}

static void
load_module(uintptr_t base, struct load_info *info)
{
    memset((void*)info, 0, sizeof(struct load_info));

    setup_load_info(base, info);

    rewrite_section_headers(info);

    layout_sections(info);

    /*
    {
        char tmp[64] = {0};
        sbi_console_puts(hdr->e_ident);
        sbi_console_puts("\n");
        hex_to_str(hdr->e_shoff, tmp, sizeof(tmp));
        sbi_console_puts(tmp);
        sbi_console_puts("\n");
    }
    */
}

static uintptr_t
modules_source_base(void)
{
    uintptr_t base = (FLASH_VA + FLASH_HEAD_SIZE);
    struct image_header *hdr = (struct image_header *) base;

    return ROUND_UP((base + hdr->res2), 8);
}

static void
move_module(uintptr_t addr, struct load_info *info)
{
    int i;

    addr = ROUND_UP(addr, 8);

    memset((void*)addr, 0, info->layout.size);

    for (i = 0; i < info->hdr->e_shnum; i++) {
        void *p;
        Elf64_Shdr *s = info->sechdrs + i;

        if (!(s->sh_flags & SHF_ALLOC))
            continue;

        p = (void*)addr + s->sh_entsize;

        if (s->sh_type != SHT_NOBITS)
            memcpy(p, (void *)s->sh_addr, s->sh_size);

        /* Update sh_addr to point to copy in image. */
        s->sh_addr = (unsigned long)p;
    }
}

static void
simplify_symbols(const struct load_info *info)
{
    int i;
    Elf64_Shdr *symsec = &info->sechdrs[info->index.sym];
    Elf64_Sym *sym = (void *)symsec->sh_addr;

    for (i = 1; i < symsec->sh_size / sizeof(Elf64_Sym); i++) {
        const char *name = info->strtab + sym[i].st_name;

        switch (sym[i].st_shndx) {
        case SHN_COMMON:
        case SHN_ABS:
        case SHN_LIVEPATCH:
            break;
        case SHN_UNDEF:
            sbi_console_puts("[");
            sbi_console_puts(name);
            sbi_console_puts("]\n");
            break;
        default:
            sym[i].st_value += info->sechdrs[sym[i].st_shndx].sh_addr;
            /*
            {
                char tmp[64] = {0};
                hex_to_str(sym[i].st_value, tmp, sizeof(tmp));
                sbi_console_puts(tmp);
                sbi_console_puts("\n");
            }
            */
            break;
        }
    }
}

static void
apply_relocations(const struct load_info *info)
{
    int i;

    for (i = 1; i < info->hdr->e_shnum; i++) {
        unsigned int infosec = info->sechdrs[i].sh_info;
        {
            char tmp[64] = {0};
            hex_to_str(infosec, tmp, sizeof(tmp));
            sbi_console_puts(tmp);
            sbi_console_puts("\n");
        }
    }
}

static void
load_modules(void)
{
    int i;
    struct load_info info;

    uintptr_t src_addr = modules_source_base();
    uintptr_t dst_addr = (uintptr_t)_end;

    {
        char tmp[64] = {0};
        hex_to_str(dst_addr, tmp, sizeof(tmp));
        sbi_console_puts(tmp);
        sbi_console_puts("\n");
    }

    load_module(src_addr, &info);

    move_module(dst_addr, &info);

    simplify_symbols(&info);

    apply_relocations(&info);

    /* next */
    dst_addr += info.layout.size;
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
