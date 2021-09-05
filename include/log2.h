/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_LOG2_H
#define _LINUX_LOG2_H

#include <compiler_attributes.h>

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 *
 * __builtin_clz: return leading 0-bits in x,
 * from the most-significant position.
 */
static __always_inline int fls(unsigned int x)
{
    return x ? sizeof(x) * 8 - __builtin_clz(x) : 0;
}

static __always_inline int fls64(unsigned long x)
{
    return x ? sizeof(x) * 8 - __builtin_clzl(x) : 0;
}

/*
 * non-constant log of base 2 calculators
 * - the arch may override these in asm/bitops.h if they can be implemented
 *   more efficiently than using fls() and fls64()
 * - the arch is not required to handle n==0 if implementing the fallback
 */
static inline __attribute__((const))
int __ilog2_u32(u32 n)
{
    return fls(n) - 1;
}

static inline __attribute__((const))
int __ilog2_u64(u64 n)
{
    return fls64(n) - 1;
}

/**
 * const_ilog2 - log base 2 of 32-bit or a 64-bit constant unsigned value
 * @n: parameter
 *
 * Use this where sparse expects a true constant expression, e.g. for array
 * indices.
 */
#define const_ilog2(n)              \
(                       \
    __builtin_constant_p(n) ? (     \
        (n) < 2 ? 0 :           \
        (n) & (1ULL << 63) ? 63 :   \
        (n) & (1ULL << 62) ? 62 :   \
        (n) & (1ULL << 61) ? 61 :   \
        (n) & (1ULL << 60) ? 60 :   \
        (n) & (1ULL << 59) ? 59 :   \
        (n) & (1ULL << 58) ? 58 :   \
        (n) & (1ULL << 57) ? 57 :   \
        (n) & (1ULL << 56) ? 56 :   \
        (n) & (1ULL << 55) ? 55 :   \
        (n) & (1ULL << 54) ? 54 :   \
        (n) & (1ULL << 53) ? 53 :   \
        (n) & (1ULL << 52) ? 52 :   \
        (n) & (1ULL << 51) ? 51 :   \
        (n) & (1ULL << 50) ? 50 :   \
        (n) & (1ULL << 49) ? 49 :   \
        (n) & (1ULL << 48) ? 48 :   \
        (n) & (1ULL << 47) ? 47 :   \
        (n) & (1ULL << 46) ? 46 :   \
        (n) & (1ULL << 45) ? 45 :   \
        (n) & (1ULL << 44) ? 44 :   \
        (n) & (1ULL << 43) ? 43 :   \
        (n) & (1ULL << 42) ? 42 :   \
        (n) & (1ULL << 41) ? 41 :   \
        (n) & (1ULL << 40) ? 40 :   \
        (n) & (1ULL << 39) ? 39 :   \
        (n) & (1ULL << 38) ? 38 :   \
        (n) & (1ULL << 37) ? 37 :   \
        (n) & (1ULL << 36) ? 36 :   \
        (n) & (1ULL << 35) ? 35 :   \
        (n) & (1ULL << 34) ? 34 :   \
        (n) & (1ULL << 33) ? 33 :   \
        (n) & (1ULL << 32) ? 32 :   \
        (n) & (1ULL << 31) ? 31 :   \
        (n) & (1ULL << 30) ? 30 :   \
        (n) & (1ULL << 29) ? 29 :   \
        (n) & (1ULL << 28) ? 28 :   \
        (n) & (1ULL << 27) ? 27 :   \
        (n) & (1ULL << 26) ? 26 :   \
        (n) & (1ULL << 25) ? 25 :   \
        (n) & (1ULL << 24) ? 24 :   \
        (n) & (1ULL << 23) ? 23 :   \
        (n) & (1ULL << 22) ? 22 :   \
        (n) & (1ULL << 21) ? 21 :   \
        (n) & (1ULL << 20) ? 20 :   \
        (n) & (1ULL << 19) ? 19 :   \
        (n) & (1ULL << 18) ? 18 :   \
        (n) & (1ULL << 17) ? 17 :   \
        (n) & (1ULL << 16) ? 16 :   \
        (n) & (1ULL << 15) ? 15 :   \
        (n) & (1ULL << 14) ? 14 :   \
        (n) & (1ULL << 13) ? 13 :   \
        (n) & (1ULL << 12) ? 12 :   \
        (n) & (1ULL << 11) ? 11 :   \
        (n) & (1ULL << 10) ? 10 :   \
        (n) & (1ULL <<  9) ?  9 :   \
        (n) & (1ULL <<  8) ?  8 :   \
        (n) & (1ULL <<  7) ?  7 :   \
        (n) & (1ULL <<  6) ?  6 :   \
        (n) & (1ULL <<  5) ?  5 :   \
        (n) & (1ULL <<  4) ?  4 :   \
        (n) & (1ULL <<  3) ?  3 :   \
        (n) & (1ULL <<  2) ?  2 :   \
        1) :                \
    -1)

/**
 * ilog2 - log base 2 of 32-bit or a 64-bit unsigned value
 * @n: parameter
 *
 * constant-capable log of base 2 calculation
 * - this can be used to initialise global variables from constant data, hence
 * the massive ternary operator construction
 *
 * selects the appropriately-sized optimised version depending on sizeof(n)
 */
#define ilog2(n) \
( \
    __builtin_constant_p(n) ?   \
    const_ilog2(n) :        \
    (sizeof(n) <= 4) ?      \
    __ilog2_u32(n) :        \
    __ilog2_u64(n)          \
)

static inline __attribute_const__
int __order_base_2(unsigned long n)
{
    return n > 1 ? ilog2(n - 1) + 1 : 0;
}

/**
 * order_base_2 - calculate the (rounded up) base 2 order of the argument
 * @n: parameter
 *
 * The first few values calculated by this routine:
 *  ob2(0) = 0
 *  ob2(1) = 0
 *  ob2(2) = 1
 *  ob2(3) = 2
 *  ob2(4) = 2
 *  ob2(5) = 3
 *  ... and so on.
 */
#define order_base_2(n)             \
(                       \
    __builtin_constant_p(n) ? (     \
        ((n) == 0 || (n) == 1) ? 0 :    \
        ilog2((n) - 1) + 1) :       \
    __order_base_2(n)           \
)

#endif /* _LINUX_LOG2_H */
