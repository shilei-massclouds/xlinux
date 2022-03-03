/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_RISCV_UACCESS_H
#define _ASM_RISCV_UACCESS_H

#include <csr.h>
#include <string.h>
#include <thread_info.h>

#define RISCV_PTR       ".dword"
#define RISCV_SZPTR     "8"

#define get_fs() (current_thread_info()->addr_limit)

static inline void set_fs(mm_segment_t fs)
{
    current_thread_info()->addr_limit = fs;
}

#define uaccess_kernel() (get_fs().seg == KERNEL_DS.seg)

#define access_ok(addr, size) ({                    \
    likely(__access_ok((unsigned long __force)(addr), (size))); \
})

/*
 * Ensure that the range [addr, addr+size) is within the process's
 * address space
 */
static inline int __access_ok(unsigned long addr, unsigned long size)
{
    const mm_segment_t fs = get_fs();

    return size <= fs.seg && addr <= fs.seg - size;
}

/**
 * put_user: - Write a simple value into user space.
 * @x:   Value to copy to user space.
 * @ptr: Destination address, in user space.
 *
 * Context: User context only.  This function may sleep.
 *
 * This macro copies a single simple value from kernel space to user
 * space.  It supports simple types like char and int, but not larger
 * data types like structures or arrays.
 *
 * @ptr must have pointer-to-simple-variable type, and @x must be assignable
 * to the result of dereferencing @ptr.
 *
 * Returns zero on success, or -EFAULT on error.
 */
#define put_user(x, ptr)                    \
({                              \
    __typeof__(*(ptr)) *__p = (ptr);         \
    access_ok(__p, sizeof(*__p)) ?      \
        __put_user((x), __p) :              \
        -EFAULT;                    \
})

#define __enable_user_access()                          \
    __asm__ __volatile__ ("csrs sstatus, %0" : : "r" (SR_SUM) : "memory")
#define __disable_user_access()                         \
    __asm__ __volatile__ ("csrc sstatus, %0" : : "r" (SR_SUM) : "memory")

#define __put_user_asm(insn, x, ptr, err)           \
do {                                \
    uintptr_t __tmp;                    \
    __typeof__(*(ptr)) __x = x;             \
    __enable_user_access();                 \
    __asm__ __volatile__ (                  \
        "   " insn " %z3, %2\n"         \
        : "+r" (err), "=r" (__tmp), "=m" (*(ptr))   \
        : "rJ" (__x), "i" (-EFAULT));           \
    __disable_user_access();                \
} while (0)

#define __put_user_8(x, ptr, err) __put_user_asm("sd", x, ptr, err)

#define __put_user(x, ptr)                  \
({                              \
    register long __pu_err = 0;             \
    __typeof__(*(ptr)) *__gu_ptr = (ptr);        \
    switch (sizeof(*__gu_ptr)) {                \
    case 1:                         \
        __put_user_asm("sb", (x), __gu_ptr, __pu_err);  \
        break;                      \
    case 2:                         \
        __put_user_asm("sh", (x), __gu_ptr, __pu_err);  \
        break;                      \
    case 4:                         \
        __put_user_asm("sw", (x), __gu_ptr, __pu_err);  \
        break;                      \
    case 8:                         \
        __put_user_8((x), __gu_ptr, __pu_err);  \
        break;                      \
    default:                        \
        BUG();                    \
    }                           \
    __pu_err;                       \
})

/*
 * Return the size of a string (including the ending 0)
 *
 * Return 0 on exception, a value greater than N if too long
 */
#ifndef __strnlen_user
#define __strnlen_user(s, n) (strnlen((s), (n)) + 1)
#endif

/*
 * Unlike strnlen, strnlen_user includes the nul terminator in
 * its returned count. Callers should check for a returned value
 * greater than N as an indication the string is too long.
 */
static inline long strnlen_user(const char *src, long n)
{
    if (!access_ok(src, 1))
        return 0;
    return __strnlen_user(src, n);
}

extern unsigned long
asm_copy_to_user(void *to, const void *from, unsigned long n);

static inline unsigned long
raw_copy_to_user(void *to, const void *from, unsigned long n)
{
    return asm_copy_to_user(to, from, n);
}

static inline unsigned long
_copy_to_user(void *to, const void *from, unsigned long n)
{
    if (access_ok(to, n))
        n = raw_copy_to_user(to, from, n);
    return n;
}

static __always_inline unsigned long
copy_to_user(void *to, const void *from, unsigned long n)
{
    return _copy_to_user(to, from, n);
}

/*
 * Force the uaccess routines to be wired up for actual userspace access,
 * overriding any possible set_fs(KERNEL_DS) still lingering around.
 * Undone using force_uaccess_end below.
 */
static inline mm_segment_t force_uaccess_begin(void)
{
    mm_segment_t fs = get_fs();

    set_fs(USER_DS);
    return fs;
}

static inline unsigned long
__clear_user(void *to, unsigned long n)
{
    memset((void __force *)to, 0, n);
    return 0;
}

static inline
unsigned long clear_user(void *to, unsigned long n)
{
    return access_ok(to, n) ?  __clear_user(to, n) : n;
}

#endif /* _ASM_RISCV_UACCESS_H */
