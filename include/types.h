/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#ifndef __ASSEMBLY__

extern const unsigned char _ctype[];

#define _U  0x01    /* upper */
#define _L  0x02    /* lower */
#define _D  0x04    /* digit */
#define _C  0x08    /* cntrl */
#define _P  0x10    /* punct */
#define _S  0x20    /* white space (space/lf/tab) */
#define _X  0x40    /* hex digit */
#define _SP 0x80    /* hard space (0x20) */

#define __ismask(x) (_ctype[(int)(unsigned char)(x)])

#define islower(c)  ((__ismask(c)&(_L)) != 0)
#define isupper(c)  ((__ismask(c)&(_U)) != 0)

#define NULL ((void *)0)

#define SIZE_MAX        (~(size_t)0)
#define PHYS_ADDR_MAX   (~(phys_addr_t)0)
#define INT_MAX         ((int)(~0U >> 1))
#define ULLONG_MAX      (~0ULL)

#define min(a, b)   ((a < b) ? a : b)
#define max(a, b)   ((a > b) ? a : b)

#define min3(x, y, z) min((typeof(x))min(x, y), z)

#define clamp(val, lo, hi) min((typeof(val))max(val, lo), hi)

#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_down(x, y) ((x) & ~__round_mask(x, y))

#define __typecheck(x, y) \
        (!!(sizeof((typeof(x) *)1 == (typeof(y) *)1)))

#define __is_constexpr(x) \
    (sizeof(int) == sizeof(*(8 ? ((void *)((long)(x) * 0l)) : (int *)8)))

#define __no_side_effects(x, y) \
        (__is_constexpr(x) && __is_constexpr(y))

#define __safe_cmp(x, y) \
        (__typecheck(x, y) && __no_side_effects(x, y))

#define __cmp(x, y, op) ((x) op (y) ? (x) : (y))

#define __cmp_once(x, y, unique_x, unique_y, op) ({ \
        typeof(x) unique_x = (x);       \
        typeof(y) unique_y = (y);       \
        __cmp(unique_x, unique_y, op); })

#define ___PASTE(a,b) a##b
#define __PASTE(a,b) ___PASTE(a,b)

#define __UNIQUE_ID(prefix) \
    __PASTE(__PASTE(__UNIQUE_ID_, prefix), __COUNTER__)

#define __careful_cmp(x, y, op) \
    __builtin_choose_expr(__safe_cmp(x, y), \
        __cmp(x, y, op), \
        __cmp_once(x, y, __UNIQUE_ID(__x), __UNIQUE_ID(__y), op))

/**
 * min_t - return minimum of two values, using the specified type
 * @type: data type to use
 * @x: first value
 * @y: second value
 */
#define min_t(type, x, y)   __careful_cmp((type)(x), (type)(y), <)

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

typedef unsigned int    __kernel_uid32_t;
typedef unsigned int    __kernel_gid32_t;

typedef __kernel_uid32_t    uid_t;
typedef __kernel_gid32_t    gid_t;

typedef phys_addr_t resource_size_t;

typedef unsigned int gfp_t;
typedef unsigned int slab_flags_t;

#endif /*  __ASSEMBLY__ */

#define swab32(x) ((u32)(                    \
    (((u32)(x) & (u32)0x000000ffUL) << 24) | \
    (((u32)(x) & (u32)0x0000ff00UL) <<  8) | \
    (((u32)(x) & (u32)0x00ff0000UL) >>  8) | \
    (((u32)(x) & (u32)0xff000000UL) >> 24)))

#define be32_to_cpu(x)  swab32((u32)(x))
#define be32_to_cpup(x) swab32(*((u32*)(x)))
#define cpu_to_be32(x)  ((u32)swab32((x)))

#define _ALIGN_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define _ALIGN(x, a)            _ALIGN_MASK((x), (typeof(x))(a) - 1)
#define _ALIGN_DOWN(x, a)       _ALIGN((x) - ((a) - 1), (a))
#define PTR_ALIGN(p, a)         ((typeof(p))_ALIGN((unsigned long)(p), (a)))
#define PTR_ALIGN_DOWN(p, a)    ((typeof(p))_ALIGN_DOWN((unsigned long)(p), (a)))

#define IS_ALIGNED(x, a)    (((x) & ((typeof(x))(a) - 1)) == 0)

#endif /* _LINUX_TYPES_H */
