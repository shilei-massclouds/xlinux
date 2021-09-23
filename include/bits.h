/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _GENERIC_BITS_H_
#define _GENERIC_BITS_H_

#define BITS_PER_LONG       64
#define BITS_PER_LONG_LONG  64

#define BIT_ULL(nr)         (1ULL << (nr))
#define BIT_MASK(nr)        (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)        ((nr) / BITS_PER_LONG)
#define BIT_ULL_MASK(nr)    (1ULL << ((nr) % BITS_PER_LONG_LONG))
#define BIT_ULL_WORD(nr)    ((nr) / BITS_PER_LONG_LONG)
#define BITS_PER_BYTE       8

static inline void
set_bit(unsigned int nr, volatile unsigned long *p)
{
    p += BIT_WORD(nr);
    *p |= BIT_MASK(nr);
}

static inline void
clear_bit(unsigned int nr, volatile unsigned long *p)
{
    p += BIT_WORD(nr);
    *p &= ~BIT_MASK(nr);
}

static inline int
test_bit(int nr, const volatile unsigned long *addr)
{
    return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}

static inline int
test_and_set_bit(unsigned int nr, volatile unsigned long *p)
{
    long old;
    unsigned long mask = BIT_MASK(nr);

    p += BIT_WORD(nr);
    if (*p & mask)
        return 1;

    old = *p;
    *p |= mask;
    return !!(old & mask);
}


#endif /* _GENERIC_BITS_H_ */
