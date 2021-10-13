/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_KERNEL_H
#define _UAPI_LINUX_KERNEL_H

#include <types.h>

#define MAX_NUMNODES    1

#define ALIGN(x, a) _ALIGN((x), (a))

#define L1_CACHE_SHIFT  6
#define L1_CACHE_BYTES  (1 << L1_CACHE_SHIFT)
#define SMP_CACHE_BYTES L1_CACHE_BYTES
#define cache_line_size()   L1_CACHE_BYTES

/**
 * ARRAY_SIZE - get the number of elements in array @arr
 * @arr: array to be sized
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define MAX_ERRNO   4095
#define IS_ERR_VALUE(x) \
    ((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

#define _RET_IP_    (unsigned long)__builtin_return_address(0)

static inline bool
IS_ERR(const void *ptr)
{
    return IS_ERR_VALUE((unsigned long)ptr);
}

static inline bool
IS_ERR_OR_NULL(const void *ptr)
{
    return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}

typedef void (*start_kernel_t)(void);
extern start_kernel_t start_kernel_fn;

static inline void
local_flush_tlb_page(unsigned long addr)
{
    __asm__ __volatile__ ("sfence.vma %0" : : "r" (addr) : "memory");
}

static inline void local_flush_tlb_all(void)
{
    __asm__ __volatile__ ("sfence.vma" : : : "memory");
}

#endif /* _UAPI_LINUX_KERNEL_H */
