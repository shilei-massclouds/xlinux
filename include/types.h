/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#ifndef __ASSEMBLY__

#define NULL ((void *)0)

#define SIZE_MAX        (~(size_t)0)
#define PHYS_ADDR_MAX   (~(phys_addr_t)0)
#define ULLONG_MAX      (~0ULL)

#define min(a, b)   ((a < b) ? a : b)
#define max(a, b)   ((a > b) ? a : b)

#define clamp(val, lo, hi) min((typeof(val))max(val, lo), hi)

#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_down(x, y) ((x) & ~__round_mask(x, y))

typedef struct {
    int counter;
} atomic_t;

typedef unsigned long   uintptr_t;

typedef _Bool           bool;

enum {
    false   = 0,
    true    = 1
};

typedef int     s32;
typedef long    s64;

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned long   u64;

typedef u8  uint8_t;
typedef u16 uint16_t;
typedef u32 uint32_t;
typedef u64 uint64_t;

typedef u64 phys_addr_t;

typedef long                __kernel_long_t;
typedef unsigned long       __kernel_ulong_t;
typedef __kernel_ulong_t    __kernel_size_t;

typedef __kernel_long_t     __kernel_ptrdiff_t;
typedef __kernel_ptrdiff_t  ptrdiff_t;

#ifndef _SIZE_T
#define _SIZE_T
typedef __kernel_size_t     size_t;
#endif

extern void *memset(void *, int, __kernel_size_t);
extern void *memcpy(void *, const void *, size_t);
extern int memcmp(const void *cs, const void *ct, size_t count);
extern void *memchr(const void *s, int c, size_t n);
extern void *memmove(void *dest, const void *src, size_t count);
extern int strcmp(const char *cs, const char *ct);
extern size_t strlen(const char *s);

#endif /*  __ASSEMBLY__ */

#define swab32(x) ((u32)(                    \
    (((u32)(x) & (u32)0x000000ffUL) << 24) | \
    (((u32)(x) & (u32)0x0000ff00UL) <<  8) | \
    (((u32)(x) & (u32)0x00ff0000UL) >>  8) | \
    (((u32)(x) & (u32)0xff000000UL) >> 24)))

#define be32_to_cpu(x) swab32((u32)(x))
#define be32_to_cpup(x) swab32(*((u32*)(x)))

#define ALIGN_MASK(x, mask)  (((x) + (mask)) & ~(mask))
#define ALIGN(x, a) ALIGN_MASK((x), (typeof(x))(a) - 1)
#define IS_ALIGNED(x, a)    (((x) & ((typeof(x))(a) - 1)) == 0)

#endif /* _LINUX_TYPES_H */
