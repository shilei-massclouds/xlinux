// SPDX-License-Identifier: GPL-2.0

#ifndef _ASM_GENERIC_ATOMIC_LONG_H
#define _ASM_GENERIC_ATOMIC_LONG_H

#include <bug.h>
#include <types.h>
#include <compiler_attributes.h>

/*
 * Use __READ_ONCE() instead of READ_ONCE() if you do not require any
 * atomicity. Note that this may result in tears!
 */
#ifndef __READ_ONCE
#define __READ_ONCE(x)  (*(const volatile __unqual_scalar_typeof(x) *)&(x))
#endif

#define READ_ONCE(x)    \
({                      \
    __READ_ONCE(x);     \
})

#define __WRITE_ONCE(x, val)                        \
do {                                    \
    *(volatile typeof(x) *)&(x) = (val);                \
} while (0)

#define WRITE_ONCE(x, val)  \
do {                        \
    __WRITE_ONCE(x, val);   \
} while (0)

#define atomic_read(v)  READ_ONCE((v)->counter)
#define atomic_set(v, i) WRITE_ONCE(((v)->counter), (i))

typedef struct {
    int counter;
} atomic_t;

typedef struct {
    s64 counter;
} atomic64_t;

typedef atomic64_t atomic_long_t;

static __always_inline s64
atomic64_read(const atomic64_t *v)
{
    return READ_ONCE(v->counter);
}

static __always_inline void
atomic64_set(atomic64_t *v, s64 i)
{
    WRITE_ONCE(v->counter, i);
}

static __always_inline void
atomic_long_set(atomic_long_t *v, long i)
{
    atomic64_set(v, i);
}

#endif /* _ASM_GENERIC_ATOMIC_LONG_H */
