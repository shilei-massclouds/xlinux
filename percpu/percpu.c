// SPDX-License-Identifier: GPL-2.0-only
#include <percpu.h>
#include <export.h>
#include <printk.h>

void *
__alloc_percpu(size_t size, size_t align)
{
    printk("percpu ...\n");
    return NULL;
    //return pcpu_alloc(size, align, false, GFP_KERNEL);
}
EXPORT_SYMBOL(__alloc_percpu);
