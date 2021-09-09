/* SPDX-License-Identifier: GPL-2.0-only */

#include <types.h>
#include <module.h>
#include <sbi.h>
#include <image.h>
#include <fixmap.h>
#include <elf.h>
#include <kernel.h>
#include <bug.h>

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
            sbi_console_printf("SHN_UNDEF: %s: %lx\n",
                               name, sym[i].st_shndx);
            break;
        default:
            sym[i].st_value += info->sechdrs[sym[i].st_shndx].sh_addr;
            sbi_console_printf("%lx\n", sym[i].st_value);
            break;
        }
    }
}

static void
apply_relocate_add(Elf64_Shdr *sechdrs, const char *strtab,
                   unsigned int symindex, unsigned int relsec)
{
    int i;
    u64 *location;
    Elf64_Sym *sym;
    unsigned int type;
    u64 v;
    Elf64_Shdr *shdr = sechdrs + relsec;
    Elf64_Rela *rel = (void *)shdr->sh_addr;

    for (i = 0; i < shdr->sh_size / sizeof(*rel); i++) {
        /* This is where to make the change */
        location = (void *)sechdrs[shdr->sh_info].sh_addr + rel[i].r_offset;

        /* This is the symbol it is referring to */
        sym = (Elf64_Sym *)sechdrs[symindex].sh_addr +
            ELF64_R_SYM(rel[i].r_info);
        BUG_ON(IS_ERR_VALUE(sym->st_value));

        v = sym->st_value + rel[i].r_addend;

        type = ELF64_R_TYPE(rel[i].r_info);
        switch (type) {
        case R_RISCV_64:
            *location = v;
            break;
        case R_RISCV_PCREL_HI20: {
            ptrdiff_t offset = (void *)v - (void *)location;
            s32 hi20 = (offset + 0x800) & 0xfffff000;
            *location = (*location & 0xfff) | hi20;
            break;
        }
        case R_RISCV_CALL: {
            ptrdiff_t offset = (void *)v - (void *)location;
            s32 fill_v = offset;
            u32 hi20, lo12;

            hi20 = (offset + 0x800) & 0xfffff000;
            lo12 = (offset - hi20) & 0xfff;
            *location = (*location & 0xfff) | hi20;
            *(location + 1) = (*(location + 1) & 0xfffff) | (lo12 << 20);
            break;
        }
        case R_RISCV_RELAX:
            break;
        case R_RISCV_PCREL_LO12_I:
        case R_RISCV_PCREL_LO12_S: {
            int j;
            for (j = 0; j < shdr->sh_size / sizeof(*rel); j++) {
                u32 hi20_type = ELF64_R_TYPE(rel[j].r_info);
                u64 hi20_loc = sechdrs[shdr->sh_info].sh_addr +
                    rel[j].r_offset;

                BUG_ON(hi20_type == R_RISCV_GOT_HI20);
                if (hi20_loc == sym->st_value &&
                    hi20_type == R_RISCV_PCREL_HI20) {
                    s32 hi20, lo12;
                    Elf64_Sym *hi20_sym =
                        (Elf64_Sym *)sechdrs[symindex].sh_addr +
                        ELF64_R_SYM(rel[j].r_info);

                    unsigned long hi20_sym_val = hi20_sym->st_value
                        + rel[j].r_addend;

                    size_t offset = hi20_sym_val - hi20_loc;

                    hi20 = (offset + 0x800) & 0xfffff000;
                    lo12 = offset - hi20;
                    *location = lo12;
                    break;
                }
            }
            break;
        }
        default:
            sbi_console_printf("[%s]: \n", __func__);
            BUG_ON(type);
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

        /* Not a valid relocation section? */
        if (infosec >= info->hdr->e_shnum)
            continue;

        /* Don't bother with non-allocated sections */
        if (!(info->sechdrs[infosec].sh_flags & SHF_ALLOC))
            continue;

        if (info->sechdrs[i].sh_type == SHT_RELA)
            apply_relocate_add(info->sechdrs,
                               info->strtab, info->index.sym, i);
    }
}

static void
init_other_modules(void)
{
    int i;
    struct load_info info;

    uintptr_t src_addr = modules_source_base();
    uintptr_t dst_addr = (uintptr_t)_end;

    load_module(src_addr, &info);

    move_module(dst_addr, &info);

    simplify_symbols(&info);

    apply_relocations(&info);

    /* next */
    dst_addr += info.layout.size;
}

void load_modules(void)
{
    sbi_console_printf("%s: init ... \n", __func__);

    init_kernel_module();

    init_other_modules();
}
