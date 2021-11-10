/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_RISCV_MMIO_H
#define _ASM_RISCV_MMIO_H

#include <types.h>

#define readb_cpu(c)    ({ u8  __r = __raw_readb(c); __r; })
#define readw_cpu(c)    ({ u16 __r = (__raw_readw(c)); __r; })
#define readl_cpu(c)    ({ u32 __r = (__raw_readl(c)); __r; })

#define readb(c)    ({ u8  __v; __v = readb_cpu(c); __v; })
#define readw(c)    ({ u16 __v; __v = readw_cpu(c); __v; })
#define readl(c)    ({ u32 __v; __v = readl_cpu(c); __v; })

#define writeb(v, c)    ({ __io_bw(); writeb_cpu((v), (c)); __io_aw(); })
#define writew(v, c)    ({ __io_bw(); writew_cpu((v), (c)); __io_aw(); })
#define writel(v, c)    ({ __io_bw(); writel_cpu((v), (c)); __io_aw(); })

static inline u8 __raw_readb(const volatile void *addr)
{
    u8 val;

    asm volatile("lb %0, 0(%1)" : "=r" (val) : "r" (addr));
    return val;
}

static inline u16 __raw_readw(const volatile void *addr)
{
    u16 val;

    asm volatile("lh %0, 0(%1)" : "=r" (val) : "r" (addr));
    return val;
}

static inline u32 __raw_readl(const volatile void *addr)
{
    u32 val;

    asm volatile("lw %0, 0(%1)" : "=r" (val) : "r" (addr));
    return val;
}

#endif /* _ASM_RISCV_MMIO_H */
