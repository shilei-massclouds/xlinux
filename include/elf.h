/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_ELF_H
#define _UAPI_LINUX_ELF_H

#include <types.h>

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

#endif /* _UAPI_LINUX_ELF_H */
