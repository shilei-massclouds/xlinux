/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_REFCOUNT_H
#define _LINUX_REFCOUNT_H

#include <types.h>

typedef struct refcount_struct {
    atomic_t refs;
} refcount_t;

/**
 * refcount_set - set a refcount's value
 * @r: the refcount
 * @n: value to which the refcount will be set
 */
static inline void
refcount_set(refcount_t *r, int n)
{
    atomic_set(&r->refs, n);
}

#endif /* _LINUX_REFCOUNT_H */
