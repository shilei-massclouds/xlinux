/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_KERNEL_H
#define _UAPI_LINUX_KERNEL_H

#define ALIGN_KERNEL(x, a)  ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define ALIGN_KERNEL_MASK(x, mask)  (((x) + (mask)) & ~(mask))

#define ALIGN(x, a) ALIGN_KERNEL((x), (a))

/**
 * ARRAY_SIZE - get the number of elements in array @arr
 * @arr: array to be sized
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define MAX_ERRNO   4095
#define IS_ERR_VALUE(x) \
    ((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

static inline bool
IS_ERR(const void *ptr)
{
    return IS_ERR_VALUE((unsigned long)ptr);
}

#endif /* _UAPI_LINUX_KERNEL_H */
