/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_RISCV_BUG_H
#define _ASM_RISCV_BUG_H

#include <sbi.h>

#define BUG() do {    \
    sbi_printf("\n########################\n"); \
    sbi_printf("BUG: %s (%s:%u)", __FUNCTION__, __FILE__, __LINE__); \
    sbi_printf("\n########################\n"); \
    __asm__ __volatile__ ("ebreak\n");  \
} while (0)

#define BUG_ON(cond) do {   \
    if ((cond)) BUG();      \
} while (0)

#define panic(args...) \
    do { \
        sbi_printf("\n########################\n"); \
        sbi_printf("PANIC: %s (%s:%u)\n", __FUNCTION__, __FILE__, __LINE__); \
        sbi_printf(args); \
        sbi_printf("\n########################\n"); \
        BUG(); \
    } while(0)

#endif /* _ASM_RISCV_BUG_H */
