/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_KERNEL_H
#define _UAPI_LINUX_KERNEL_H

#define NULL ((void *)0)

#define ALIGN_KERNEL(x, a)  ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define ALIGN_KERNEL_MASK(x, mask)  (((x) + (mask)) & ~(mask))

#define ALIGN(x, a) ALIGN_KERNEL((x), (a))

/**
 * ARRAY_SIZE - get the number of elements in array @arr
 * @arr: array to be sized
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif /* _UAPI_LINUX_KERNEL_H */
