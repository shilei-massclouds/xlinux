/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_ELF_H
#define _UAPI_LINUX_ELF_H

#include <types.h>

/* sh_flags */
#define SHF_WRITE           0x1
#define SHF_ALLOC           0x2
#define SHF_EXECINSTR       0x4
#define SHF_RELA_LIVEPATCH  0x00100000
#define SHF_RO_AFTER_INIT   0x00200000
#define SHF_MASKPROC        0xf0000000

/* sh_type */
#define SHT_NULL    0
#define SHT_PROGBITS    1
#define SHT_SYMTAB  2
#define SHT_STRTAB  3
#define SHT_RELA    4
#define SHT_HASH    5
#define SHT_DYNAMIC 6
#define SHT_NOTE    7
#define SHT_NOBITS  8
#define SHT_REL     9
#define SHT_SHLIB   10
#define SHT_DYNSYM  11
#define SHT_NUM     12
#define SHT_LOPROC  0x70000000
#define SHT_HIPROC  0x7fffffff
#define SHT_LOUSER  0x80000000
#define SHT_HIUSER  0xffffffff

#define EI_NIDENT   16

typedef struct elf64_hdr {
  unsigned char e_ident[EI_NIDENT]; /* ELF "magic number" */
  u16 e_type;
  u16 e_machine;
  u32 e_version;
  u64 e_entry;          /* Entry point virtual address */
  u64 e_phoff;          /* Program header table file offset */
  u64 e_shoff;          /* Section header table file offset */
  u32 e_flags;
  u16 e_ehsize;
  u16 e_phentsize;
  u16 e_phnum;
  u16 e_shentsize;
  u16 e_shnum;
  u16 e_shstrndx;
} Elf64_Ehdr;

typedef struct elf64_shdr {
  u32 sh_name;      /* Section name, index in string tbl */
  u32 sh_type;      /* Type of section */
  u64 sh_flags;     /* Miscellaneous section attributes */
  u64 sh_addr;      /* Section virtual addr at execution */
  u64 sh_offset;    /* Section file offset */
  u64 sh_size;      /* Size of section in bytes */
  u32 sh_link;      /* Index of another section */
  u32 sh_info;      /* Additional section information */
  u64 sh_addralign; /* Section alignment */
  u64 sh_entsize;   /* Entry size if section holds table */
} Elf64_Shdr;

#endif /* _UAPI_LINUX_ELF_H */
