// SPDX-License-Identifier: GPL-2.0-only
#define X_DEBUG
#include <mm.h>
#include <bug.h>
#include <page.h>
#include <errno.h>
#include <kernel.h>
#include <export.h>
#include <string.h>
#include <printk.h>
#include <memblock.h>
#include <mmzone.h>

#define INIT_MEMBLOCK_REGIONS           128
#define INIT_MEMBLOCK_RESERVED_REGIONS  INIT_MEMBLOCK_REGIONS

void (*reserve_bootmem_region_fn)(phys_addr_t, phys_addr_t);
EXPORT_SYMBOL(reserve_bootmem_region_fn);

extern phys_addr_t dt_memory_base;
extern phys_addr_t dt_memory_size;

struct pglist_data contig_page_data;
EXPORT_SYMBOL(contig_page_data);

unsigned long max_mapnr;
EXPORT_SYMBOL(max_mapnr);

struct page *mem_map;
EXPORT_SYMBOL(mem_map);

static struct memblock_region
memblock_memory_init_regions[INIT_MEMBLOCK_REGIONS];

static struct memblock_region
memblock_reserved_init_regions[INIT_MEMBLOCK_RESERVED_REGIONS];

struct memblock memblock = {
    .memory.regions     = memblock_memory_init_regions,
    .memory.cnt         = 1,    /* empty dummy entry */
    .memory.max         = INIT_MEMBLOCK_REGIONS,
    .memory.name        = "memory",

    .reserved.regions   = memblock_reserved_init_regions,
    .reserved.cnt       = 1,    /* empty dummy entry */
    .reserved.max       = INIT_MEMBLOCK_RESERVED_REGIONS,
    .reserved.name      = "reserved",

    .bottom_up          = false,
    .current_limit      = MEMBLOCK_ALLOC_ANYWHERE,
};

static inline phys_addr_t
memblock_cap_size(phys_addr_t base, phys_addr_t *size)
{
	return *size = min(*size, PHYS_ADDR_MAX - base);
}

static void
memblock_insert_region(struct memblock_type *type, int idx,
                       phys_addr_t base, phys_addr_t size,
                       enum memblock_flags flags)
{
	struct memblock_region *rgn = &type->regions[idx];

	memmove(rgn + 1, rgn, (type->cnt - idx) * sizeof(*rgn));
	rgn->base = base;
	rgn->size = size;
	rgn->flags = flags;
	type->cnt++;
	type->total_size += size;
}

static void
memblock_merge_regions(struct memblock_type *type)
{
	int i = 0;

	/* cnt never goes below 1 */
	while (i < type->cnt - 1) {
		struct memblock_region *this = &type->regions[i];
		struct memblock_region *next = &type->regions[i + 1];

		if (this->base + this->size != next->base ||
		    this->flags != next->flags) {
			i++;
			continue;
        }

		this->size += next->size;
		/* move forward from next + 1, index of which is i + 2 */
		memmove(next, next + 1, (type->cnt - (i + 2)) * sizeof(*next));
		type->cnt--;
    }
}

static int
memblock_add_range(struct memblock_type *type,
                   phys_addr_t base, phys_addr_t size,
                   enum memblock_flags flags)
{
    int idx;
    int nr_new;
	struct memblock_region *rgn;
	bool insert = false;
	phys_addr_t obase = base;
	phys_addr_t end = base + memblock_cap_size(base, &size);

	if (!size)
		return 0;

	/* initialize empty array */
	if (type->regions[0].size == 0) {
		type->regions[0].base = base;
		type->regions[0].size = size;
		type->regions[0].flags = flags;
		type->total_size = size;
		return 0;
	}

repeat:
	/*
	 * The following is executed twice.  Once with %false @insert and
	 * then with %true.  The first counts the number of regions needed
	 * to accommodate the new area. The second actually inserts them.
	 */
	base = obase;
	nr_new = 0;

	for_each_memblock_type(idx, type, rgn) {
		phys_addr_t rbase = rgn->base;
		phys_addr_t rend = rbase + rgn->size;

		if (rbase >= end)
			break;
		if (rend <= base)
			continue;

		/*
		 * @rgn overlaps.  If it separates the lower part of new
		 * area, insert that portion.
		 */
		if (rbase > base) {
			nr_new++;
			if (insert)
				memblock_insert_region(type, idx++,
                                       base, rbase - base,
                                       flags);
        }
		/* area below @rend is dealt with, forget about it */
		base = min(rend, end);
    }

	/* insert the remaining portion */
	if (base < end) {
		nr_new++;
		if (insert)
			memblock_insert_region(type, idx, base, end - base, flags);
	}

	if (!nr_new)
		return 0;

	/*
	 * If this was the first round, resize array and repeat for actual
	 * insertions; otherwise, merge and return.
	 */
	if (!insert) {
		if (type->cnt + nr_new > type->max) {
            panic("overflow\n");
            return -ENOMEM;
        }
		insert = true;
		goto repeat;
    } else {
		memblock_merge_regions(type);
		return 0;
    }
}

int
memblock_add(phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

    pr_debug("%s: [%lx-%lx]\n", __func__, base, size);

	return memblock_add_range(&memblock.memory, base, size, 0);
}
EXPORT_SYMBOL(memblock_add);

static phys_addr_t
__memblock_find_range_top_down(phys_addr_t start, phys_addr_t end,
                               phys_addr_t size, phys_addr_t align,
                               enum memblock_flags flags)
{
    u64 i;
    phys_addr_t this_start, this_end, cand;

    for_each_free_mem_range_reverse(i, flags, &this_start, &this_end) {
        this_start = clamp(this_start, start, end);
        this_end = clamp(this_end, start, end);

        if (this_end < size)
            continue;

        cand = round_down(this_end - size, align);
        if (cand >= this_start)
            return cand;
    }

    return 0;
}

static phys_addr_t
memblock_find_in_range_node(phys_addr_t size,
                            phys_addr_t align,
                            enum memblock_flags flags)
{
    phys_addr_t start = PAGE_SIZE;
    phys_addr_t end = memblock.current_limit;

    return __memblock_find_range_top_down(start, end,
                                          size, align, flags);
}

phys_addr_t
memblock_alloc_range_nid(phys_addr_t size, phys_addr_t align)
{
    phys_addr_t found;
    enum memblock_flags flags = MEMBLOCK_NONE;

    if (!align) {
        align = SMP_CACHE_BYTES;
    }

    found = memblock_find_in_range_node(size, align, flags);
    if (found == 0)
        return 0;

    if (memblock_reserve(found, size))
        return 0;

    return found;
}

static void *
memblock_alloc_internal(phys_addr_t size, phys_addr_t align)
{
    phys_addr_t alloc;

    alloc = memblock_alloc_range_nid(size, align);
    if (!alloc)
        return NULL;

    return phys_to_virt(alloc);
}

void *
memblock_alloc_try_nid(phys_addr_t size, phys_addr_t align)
{
    void *ptr;

    pr_debug("%s: %lu bytes align=%lx\n",
             __func__, (u64)size, (u64)align);

    ptr = memblock_alloc_internal(size, align);
    if (ptr)
        memset(ptr, 0, size);

    return ptr;
}
EXPORT_SYMBOL(memblock_alloc_try_nid);

void
__next_mem_range_rev(u64 *idx, enum memblock_flags flags,
                     struct memblock_type *type_a,
                     struct memblock_type *type_b,
                     phys_addr_t *out_start,
                     phys_addr_t *out_end)
{
	int idx_a = *idx & 0xffffffff;
	int idx_b = *idx >> 32;

	if (*idx == (u64)ULLONG_MAX) {
		idx_a = type_a->cnt - 1;
		if (type_b != NULL)
			idx_b = type_b->cnt;
		else
			idx_b = 0;
	}

	for (; idx_a >= 0; idx_a--) {
		struct memblock_region *m = &type_a->regions[idx_a];

		phys_addr_t m_start = m->base;
		phys_addr_t m_end = m->base + m->size;

		if (!type_b) {
			if (out_start)
				*out_start = m_start;
			if (out_end)
				*out_end = m_end;
			idx_a--;
			*idx = (u32)idx_a | (u64)idx_b << 32;
			return;
		}

		/* scan blank spaces between reservation regions */
		for (; idx_b >= 0; idx_b--) {
			struct memblock_region *r;
			phys_addr_t r_start;
			phys_addr_t r_end;

			r = &type_b->regions[idx_b];
			r_start = idx_b ? (r[-1].base + r[-1].size) : 0;
			r_end = (idx_b < type_b->cnt) ? r->base : PHYS_ADDR_MAX;

			if (r_end <= m_start)
				break;

			/* if the two regions intersect, we're done */
			if (m_end > r_start) {
				if (out_start)
					*out_start = max(m_start, r_start);
				if (out_end)
					*out_end = min(m_end, r_end);
				if (m_start >= r_start)
					idx_a--;
				else
					idx_b--;
				*idx = (u32)idx_a | (u64)idx_b << 32;
				return;
            }
        }
    }

	/* signal end of iteration */
	*idx = ULLONG_MAX;
}

int
memblock_reserve(phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

	pr_debug("%s: [%lx-%lx]\n", __func__, base, end);

	return memblock_add_range(&memblock.reserved, base, size, 0);
}

phys_addr_t
memblock_phys_alloc_range(phys_addr_t size,
                          phys_addr_t align)
{
    return memblock_alloc_range_nid(size, align);
}

phys_addr_t
memblock_start_of_DRAM(void)
{
    return memblock.memory.regions[0].base;
}
EXPORT_SYMBOL(memblock_start_of_DRAM);

phys_addr_t
memblock_end_of_DRAM(void)
{
    int idx = memblock.memory.cnt - 1;

    return (memblock.memory.regions[idx].base +
            memblock.memory.regions[idx].size);
}
EXPORT_SYMBOL(memblock_end_of_DRAM);

void
__next_mem_pfn_range(int *idx,
                     unsigned long *out_start_pfn,
                     unsigned long *out_end_pfn)
{
    struct memblock_type *type = &memblock.memory;
    struct memblock_region *r;

    while (++*idx < type->cnt) {
        r = &type->regions[*idx];

        if (PFN_UP(r->base) >= PFN_DOWN(r->base + r->size))
            continue;

        break;
    }
    if (*idx >= type->cnt) {
        *idx = -1;
        return;
    }

    if (out_start_pfn)
        *out_start_pfn = PFN_UP(r->base);
    if (out_end_pfn)
        *out_end_pfn = PFN_DOWN(r->base + r->size);
}
EXPORT_SYMBOL(__next_mem_pfn_range);

void
reset_node_managed_pages(void)
{
    struct zone *z;
    pg_data_t *pgdat = NODE_DATA(0);

    for (z = pgdat->node_zones; z < pgdat->node_zones + MAX_NR_ZONES; z++)
        atomic_long_set(&z->managed_pages, 0);
}

static int reset_managed_pages_done;

void
reset_all_zones_managed_pages(void)
{
    if (reset_managed_pages_done)
        return;

    reset_node_managed_pages();

    reset_managed_pages_done = 1;
}

static unsigned long
free_low_memory_core_early(void)
{
    u64 i;
    phys_addr_t start;
    phys_addr_t end;

    BUG_ON(reserve_bootmem_region_fn == NULL);

    for_each_reserved_mem_region(i, &start, &end)
        reserve_bootmem_region_fn(start, end);
}

/**
 * memblock_free_all - release free pages to the buddy allocator
 *
 * Return: the number of pages actually released.
 */
static unsigned long
memblock_free_all(void)
{
    unsigned long pages;

    reset_all_zones_managed_pages();

    pages = free_low_memory_core_early();
    //totalram_pages_add(pages);

    return pages;
}

void
__next_reserved_mem_region(u64 *idx,
                           phys_addr_t *out_start,
                           phys_addr_t *out_end)
{
    struct memblock_type *type = &memblock.reserved;

    if (*idx < type->cnt) {
        struct memblock_region *r = &type->regions[*idx];
        phys_addr_t base = r->base;
        phys_addr_t size = r->size;

        if (out_start)
            *out_start = base;
        if (out_end)
            *out_end = base + size - 1;

        *idx += 1;
        return;
    }

    /* signal end of iteration */
    *idx = ULLONG_MAX;
}

static int
init_module(void)
{
    printk("module[memblock]: init begin ...\n");

    free_pages_to_buddy_fn = memblock_free_all;

    if (dt_memory_base && dt_memory_size)
        memblock_add(dt_memory_base, dt_memory_size);

    setup_vm_final(memblock.memory.regions,
                   memblock.memory.cnt,
                   memblock_phys_alloc);

    printk("module[memblock]: init end!\n");
    return 0;
}
