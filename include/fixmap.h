/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 Western Digital Corporation or its affiliates.
 */

#ifndef _ASM_RISCV_FIXMAP_H
#define _ASM_RISCV_FIXMAP_H

#include <page.h>
#include <sizes.h>

enum fixed_addresses {
    FIX_HOLE,
#define FIX_FDT_SIZE    SZ_1M
    FIX_FDT_END,
    FIX_FDT = FIX_FDT_END + FIX_FDT_SIZE / PAGE_SIZE - 1,
    FIX_PTE,
    FIX_PMD,
    FIX_TEXT_POKE1,
    FIX_TEXT_POKE0,
    FIX_EARLYCON_MEM_BASE,
    __end_of_fixed_addresses
};

#define __fix_to_virt(x)    (FIXADDR_TOP - ((x) << PAGE_SHIFT))
#define __virt_to_fix(x)    ((FIXADDR_TOP - ((x)&PAGE_MASK)) >> PAGE_SHIFT)

/* Address for flash
 * The flash is used for module
 */
#define FLASH_VA    0xFFFFFFA020000000UL
#define FLASH_PA    0x0000000020000000UL
#define FLASH_HEAD_SIZE 0x100

#endif /* _ASM_RISCV_FIXMAP_H */
