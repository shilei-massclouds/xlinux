/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#ifndef __ASSEMBLY__

typedef struct {
    int counter;
} atomic_t;

typedef unsigned long   uintptr_t;

typedef _Bool           bool;

typedef unsigned short  u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned long   u64;

typedef u64 phys_addr_t;

typedef long                __kernel_long_t;
typedef unsigned long       __kernel_ulong_t;
typedef __kernel_ulong_t    __kernel_size_t;

#ifndef _SIZE_T
#define _SIZE_T
typedef __kernel_size_t     size_t;
#endif

extern void *memset(void *, int, __kernel_size_t);

#endif /*  __ASSEMBLY__ */

#endif /* _LINUX_TYPES_H */
