/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _MEMBLOCK_H
#define _MEMBLOCK_H

#include <types.h>

int
memblock_add(phys_addr_t base, phys_addr_t size);

#endif /* _MEMBLOCK_H */
