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
extern const struct kernel_symbol _end_ksymtab[];

#define ksymtab_num (_end_ksymtab - _start_ksymtab)

LIST_HEAD(modules);
EXPORT_SYMBOL(modules);

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

static void
init_kernel_module(void)
{
    kernel_module.syms = _start_ksymtab;
    kernel_module.num_syms = ksymtab_num;

    list_add_tail(&kernel_module.list, &modules);
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

    sbi_printf("%s: \n", __func__);

    for (i = 0; i < info->hdr->e_shnum; i++)
        info->sechdrs[i].sh_entsize = ~0UL;

    for (m = 0; m < ARRAY_SIZE(masks); ++m) {
        for (i = 0; i < info->hdr->e_shnum; ++i) {
            Elf64_Shdr *s = info->sechdrs + i;

            if ((s->sh_flags & masks[m][0]) != masks[m][0]
                || (s->sh_flags & masks[m][1])
                || s->sh_entsize != ~0UL)
                continue;

            s->sh_entsize = get_offset(&(info->layout.size), s);
            /*
            sbi_printf("%s: %s %lx\n", __func__,
                       info->secstrings + s->sh_name, s->sh_entsize);
                       */
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

    sbi_printf("%s: \n", __func__);

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
    info->len = info->hdr->e_phoff;
    info->sechdrs = (void *)info->hdr + info->hdr->e_shoff;
    info->secstrings = (void *)info->hdr +
        info->sechdrs[info->hdr->e_shstrndx].sh_offset;
    info->strtab = NULL;

    sbi_printf("%s: load info len(%lx) [%lx]\n",
               __func__, info->len, (uintptr_t)info->sechdrs);
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

    sbi_printf("%s: \n", __func__);

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

        /*
        sbi_printf("%s: %s %lx, %lx\n", __func__,
                   info->secstrings + s->sh_name, s->sh_addr,
                   *((u64*)p));
                   */
    }
}

static const struct kernel_symbol *
resolve_symbol(const struct load_info *info, const char *name)
{
    int i;
    struct module *mod;

    list_for_each_entry(mod, &modules, list) {
        for (i = 0; i < mod->num_syms; i++) {
            const struct kernel_symbol *ksym = mod->syms + i;
            if (!strcmp(ksym->name, name))
                return ksym;
        }
    }

    return NULL;
}

static void
simplify_symbols(const struct load_info *info)
{
    int i;
    const struct kernel_symbol *ksym;
    Elf64_Shdr *symsec = &info->sechdrs[info->index.sym];
    Elf64_Sym *sym = (void *)symsec->sh_addr;

    sbi_printf("%s: \n", __func__);

    for (i = 1; i < symsec->sh_size / sizeof(Elf64_Sym); i++) {
        const char *name = info->strtab + sym[i].st_name;

        switch (sym[i].st_shndx) {
        case SHN_COMMON:
            panic("'SHN_COMMON' isn't supported for %s", name);
            break;
        case SHN_ABS:
        case SHN_LIVEPATCH:
            break;
        case SHN_UNDEF:
            ksym = resolve_symbol(info, name);
            if (ksym && !IS_ERR(ksym)) {
                sym[i].st_value = ksym->value;
                //sbi_printf("SHN_UNDEF [%s]: %lx\n", name, sym[i].st_value);
                break;
            }

            panic("'%s' can't be resolved", name);
            break;
        default:
            sym[i].st_value += info->sechdrs[sym[i].st_shndx].sh_addr;
            //sbi_printf("%lx\n", sym[i].st_value);
            break;
        }
    }
}

static void
apply_relocate_add(Elf64_Shdr *sechdrs, const char *strtab,
                   unsigned int symindex, unsigned int relsec)
{
    int i;
    u32 *location;
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

        /*
        sbi_printf("%s: %lx, %lx(%lx), %lx\n", __func__,
                   type, location, (*location), v);
                   */

        switch (type) {
        case R_RISCV_64:
            *(u64 *)location = v;
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
        case R_RISCV_BRANCH: {
            ptrdiff_t offset = (void *)v - (void *)location;
            u32 imm12 = (offset & 0x1000) << (31 - 12);
            u32 imm11 = (offset & 0x800) >> (11 - 7);
            u32 imm10_5 = (offset & 0x7e0) << (30 - 10);
            u32 imm4_1 = (offset & 0x1e) << (11 - 4);

            *location = (*location & 0x1fff07f) | imm12 | imm11 | imm10_5 | imm4_1;
            break;
        }
        case R_RISCV_RVC_JUMP: {
            ptrdiff_t offset = (void *)v - (void *)location;
            u16 imm11 = (offset & 0x800) << (12 - 11);
            u16 imm10 = (offset & 0x400) >> (10 - 8);
            u16 imm9_8 = (offset & 0x300) << (12 - 11);
            u16 imm7 = (offset & 0x80) >> (7 - 6);
            u16 imm6 = (offset & 0x40) << (12 - 11);
            u16 imm5 = (offset & 0x20) >> (5 - 2);
            u16 imm4 = (offset & 0x10) << (12 - 5);
            u16 imm3_1 = (offset & 0xe) << (12 - 10);

            *(u16 *)location = (*(u16 *)location & 0xe003) |
                imm11 | imm10 | imm9_8 | imm7 | imm6 | imm5 | imm4 | imm3_1;
            break;
        }
        case R_RISCV_RVC_BRANCH: {
            ptrdiff_t offset = (void *)v - (void *)location;
            u16 imm8 = (offset & 0x100) << (12 - 8);
            u16 imm7_6 = (offset & 0xc0) >> (6 - 5);
            u16 imm5 = (offset & 0x20) >> (5 - 2);
            u16 imm4_3 = (offset & 0x18) << (12 - 5);
            u16 imm2_1 = (offset & 0x6) << (12 - 10);

            *(u16 *)location = (*(u16 *)location & 0xe383) |
                imm8 | imm7_6 | imm5 | imm4_3 | imm2_1;

            break;
        }
        case R_RISCV_RELAX:
            break;
        case R_RISCV_ADD32:
            *(u32 *)location += (u32)v;
            break;
        case R_RISCV_SUB32:
            *(u32 *)location -= (u32)v;
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

                    if (type == R_RISCV_PCREL_LO12_I) {
                        *location = (*location & 0xfffff) | ((lo12 & 0xfff) << 20);
                    } else {
                        u32 imm11_5 = (lo12 & 0xfe0) << (31 - 11);
                        u32 imm4_0 = (lo12 & 0x1f) << (11 - 4);
                        *location = (*location & 0x1fff07f) | imm11_5 | imm4_0;
                    }
                    break;
                }
            }
            break;
        }
        default:
            panic("bad type %u", type);
            break;
        }
    }
}

static void
apply_relocations(const struct load_info *info)
{
    int i;

    sbi_printf("%s: \n", __func__);

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

static u64
query_sym(const char *target, struct load_info *info)
{
    int i;
    Elf64_Shdr *symsec = &info->sechdrs[info->index.sym];
    Elf64_Sym *sym = (void *)symsec->sh_addr;

    for (i = 1; i < symsec->sh_size / sizeof(Elf64_Sym); i++) {
        const char *sname = info->strtab + sym[i].st_name;
        if (!strcmp(sname, target)) {
            //sbi_printf("%s: %lx\n", __func__, sym[i].st_value);
            return sym[i].st_value;
        }
    }

    return 0;
}

static struct module *
finalize_module(uintptr_t addr, struct load_info *info)
{
    int i;
    struct kernel_symbol *start;
    struct kernel_symbol *end;
    struct module *mod;

    sbi_printf("%s: \n", __func__);

    mod = (struct module *) (addr + info->layout.size);
    info->layout.size += sizeof(struct module);

    memset((void*)mod, 0, sizeof(struct module));
    INIT_LIST_HEAD(&mod->list);
    list_add_tail(&mod->list, &modules);

    start = (struct kernel_symbol *) query_sym("_start_mod_ksymtab", info);
    end = (struct kernel_symbol *) query_sym("_end_mod_ksymtab", info);

    mod->syms = start;
    mod->num_syms = end - start;

    mod->init = (init_module_t) query_sym("init_module", info);
    mod->exit = (exit_module_t) query_sym("exit_module", info);

    /*
    for (i = 0; i < info->hdr->e_shnum; ++i) {
        Elf64_Shdr *s = info->sechdrs + i;
        const char *sname = info->secstrings + s->sh_name;
        sbi_printf("%s: [%s] entsize(%lx) sh_addr(%lx)\n",
                   __func__, sname, s->sh_entsize, s->sh_addr);
    }
    */

    return mod;
}

static void
do_init_module(struct module *mod)
{
    sbi_printf("%s: \n", __func__);

    if (mod->init)
        mod->init();
}

static void
init_other_modules(void)
{
    int i;
    struct load_info info;
    struct module *mod;

    uintptr_t src_addr = modules_source_base();
    uintptr_t dst_addr = ROUND_UP((uintptr_t)_end, 8);

    while (1) {
        /* should start with "ELF" magic number */
        if (memcmp((void *)src_addr, ELFMAG, SELFMAG))
            break;

        memset((void*)&info, 0, sizeof(struct load_info));

        setup_load_info(src_addr, &info);

        rewrite_section_headers(&info);

        layout_sections(&info);

        move_module(dst_addr, &info);

        simplify_symbols(&info);

        apply_relocations(&info);

        mod = finalize_module(dst_addr, &info);

        do_init_module(mod);

        /* next */
        src_addr += ROUND_UP(info.len, 8);
        dst_addr += ROUND_UP(info.layout.size, 8);
    }

    sbi_printf("%s: ok!\n", __func__);
}

void load_modules(void)
{
    sbi_printf("%s: init ... \n", __func__);

    init_kernel_module();

    init_other_modules();

    sbi_printf("%s: load all modules!\n", __func__);
}
