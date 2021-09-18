/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _RISCV_MM_H_
#define _RISCV_MM_H_

#include <memblock.h>

typedef phys_addr_t (*phys_alloc_t)(phys_addr_t size, phys_addr_t align);

void
setup_vm_final(struct memblock_region *regions,
               unsigned long regions_cnt,
               phys_alloc_t alloc);

#endif /* _RISCV_MM_H_ */
