/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_RISCV_BUG_H
#define _ASM_RISCV_BUG_H

#include <sbi.h>

#define BUG() do {    \
    __asm__ __volatile__ ("ebreak\n");  \
} while (0)

#define BUG_ON(cond) do {   \
    if ((cond)) BUG();      \
} while (0)

static inline void
_panic(const char *msg, const char *fn, int line, const char *file)
{
    sbi_printf("\n########################\n");
    sbi_printf("PANIC: %s(%s, line:%u)\n", fn, file, line);
    sbi_printf("%s\n", msg);
    sbi_printf("########################\n");
    BUG();
}

#define panic(msg)   _panic(msg, __func__, __LINE__, __FILE__)

#endif /* _ASM_RISCV_BUG_H */
