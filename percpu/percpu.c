// SPDX-License-Identifier: GPL-2.0-only
#include <gfp.h>
#include <percpu.h>
#include <export.h>
#include <printk.h>
#include <slab.h>

static void *
pcpu_alloc(size_t size, size_t align, bool reserved, gfp_t gfp)
{
    return kzalloc(size, gfp);
}

void *
__alloc_percpu(size_t size, size_t align)
{
    return pcpu_alloc(size, align, false, GFP_KERNEL);
}
EXPORT_SYMBOL(__alloc_percpu);
