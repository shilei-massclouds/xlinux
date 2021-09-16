/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _MEMBLOCK_H
#define _MEMBLOCK_H

#include <types.h>

#define NUMA_NO_NODE    (-1)

/* We are using top down, so it is safe to use 0 here */
#define MEMBLOCK_LOW_LIMIT  0

#define MEMBLOCK_ALLOC_ACCESSIBLE   0

int
memblock_add(phys_addr_t base, phys_addr_t size);

void *
memblock_alloc_try_nid(phys_addr_t size, phys_addr_t align);

static inline void *
memblock_alloc(phys_addr_t size, phys_addr_t align)
{
    return memblock_alloc_try_nid(size, align);
}

#endif /* _MEMBLOCK_H */
