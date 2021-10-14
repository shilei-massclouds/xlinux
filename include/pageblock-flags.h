/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef PAGEBLOCK_FLAGS_H
#define PAGEBLOCK_FLAGS_H

#include <mmzone.h>

/* 3 bits required for migrate types */
#define PB_migratetype_bits 3

/* Bit indices that affect a whole block of pages */
enum pageblock_bits {
    PB_migrate,
    PB_migrate_end = PB_migrate + PB_migratetype_bits - 1,
    PB_migrate_skip,    /* If set the block is skipped by compaction */

    /*
     * Assume the bits will always align on a word. If this assumption
     * changes then get/set pageblock needs updating.
     */
    NR_PAGEBLOCK_BITS
};

#define pageblock_order     (MAX_ORDER-1)
#define pageblock_nr_pages  (1UL << pageblock_order)

#endif /* PAGEBLOCK_FLAGS_H */
