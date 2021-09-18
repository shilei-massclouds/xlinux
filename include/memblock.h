/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _MEMBLOCK_H
#define _MEMBLOCK_H

#include <types.h>

#define NUMA_NO_NODE    (-1)

/* We are using top down, so it is safe to use 0 here */
#define MEMBLOCK_LOW_LIMIT  0

#define MEMBLOCK_ALLOC_ANYWHERE (~(phys_addr_t)0)

#define MEMBLOCK_ALLOC_ACCESSIBLE   0

#define for_each_memblock(type, region)     \
    for (region = memblock.type.regions;    \
         region < (memblock.type.regions + memblock.type.cnt); \
         region++)

#define for_each_memblock_type(i, memblock_type, rgn)   \
    for (i = 0, rgn = &memblock_type->regions[0];       \
         i < memblock_type->cnt;                        \
         i++, rgn = &memblock_type->regions[i])

#define for_each_mem_range_rev(i, type_a, type_b, flags, \
                               p_start, p_end)           \
    for (i = (u64)ULLONG_MAX,                            \
         __next_mem_range_rev(&i, flags, type_a, type_b, \
                              p_start, p_end);           \
         i != (u64)ULLONG_MAX;                           \
         __next_mem_range_rev(&i, flags, type_a, type_b, \
                              p_start, p_end))

#define for_each_free_mem_range_reverse(i, flags, p_start, p_end) \
    for_each_mem_range_rev(i, &memblock.memory, &memblock.reserved, \
                           flags, p_start, p_end)

/**
 * enum memblock_flags - definition of memory region attributes
 * @MEMBLOCK_NONE: no special request
 * @MEMBLOCK_HOTPLUG: hotpluggable region
 * @MEMBLOCK_MIRROR: mirrored region
 * @MEMBLOCK_NOMAP: don't add to kernel direct mapping
 */
enum memblock_flags {
    MEMBLOCK_NONE       = 0x0,  /* No special request */
    MEMBLOCK_HOTPLUG    = 0x1,  /* hotpluggable region */
    MEMBLOCK_MIRROR     = 0x2,  /* mirrored region */
    MEMBLOCK_NOMAP      = 0x4,  /* don't add to kernel direct mapping */
};

/**
 * struct memblock_region - represents a memory region
 * @base: base address of the region
 * @size: size of the region
 * @flags: memory region attributes
 */
struct memblock_region {
    phys_addr_t base;
    phys_addr_t size;
    enum memblock_flags flags;
};

/**
 * struct memblock_type - collection of memory regions of certain type
 * @cnt: number of regions
 * @max: size of the allocated array
 * @total_size: size of all regions
 * @regions: array of regions
 * @name: the memory type symbolic name
 */
struct memblock_type {
    unsigned long cnt;
    unsigned long max;
    phys_addr_t total_size;
    struct memblock_region *regions;
    char *name;
};

/**
 * struct memblock - memblock allocator metadata
 * @bottom_up: is bottom up direction?
 * @current_limit: physical address of the current allocation limit
 * @memory: usable memory regions
 * @reserved: reserved memory regions
 */
struct memblock {
    bool bottom_up;  /* is bottom up direction? */
    phys_addr_t current_limit;
    struct memblock_type memory;
    struct memblock_type reserved;
};

int
memblock_add(phys_addr_t base, phys_addr_t size);

void *
memblock_alloc_try_nid(phys_addr_t size, phys_addr_t align);

void
__next_mem_range_rev(u64 *idx, enum memblock_flags flags,
                     struct memblock_type *type_a,
                     struct memblock_type *type_b,
                     phys_addr_t *out_start,
                     phys_addr_t *out_end);

int
memblock_reserve(phys_addr_t base, phys_addr_t size);

phys_addr_t
memblock_phys_alloc_range(phys_addr_t size, phys_addr_t align);

void
memblock_setup_vm_final(void);

static inline void *
memblock_alloc(phys_addr_t size, phys_addr_t align)
{
    return memblock_alloc_try_nid(size, align);
}

static inline phys_addr_t
memblock_phys_alloc(phys_addr_t size, phys_addr_t align)
{
	return memblock_phys_alloc_range(size, align);
}

extern struct memblock memblock;

#endif /* _MEMBLOCK_H */
