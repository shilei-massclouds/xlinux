/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_RISCV_BUG_H
#define _ASM_RISCV_BUG_H

#define BUG() do {    \
    __asm__ __volatile__ ("ebreak\n");  \
} while (0)

#define BUG_ON(cond) do {   \
    if ((cond)) BUG();      \
} while (0)

#endif /* _ASM_RISCV_BUG_H */
