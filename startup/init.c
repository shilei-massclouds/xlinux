// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <sbi.h>
#include <slab.h>
#include <sched.h>
#include <export.h>
#include <kernel.h>

/* to be mentioned only in INIT_TASK */
struct fs_struct init_fs = {
};

struct task_struct init_task __aligned(L1_CACHE_BYTES) = {
    .fs = &init_fs,
};

/*
 * Slab
 */

/* For common cache allocation based on size */
kmalloc_t kmalloc;
EXPORT_SYMBOL(kmalloc);

kfree_t kfree;
EXPORT_SYMBOL(kfree);

kmemdup_nul_t kmemdup_nul;
EXPORT_SYMBOL(kmemdup_nul);

/* For specific cache allocation */
kmem_cache_alloc_t kmem_cache_alloc;
EXPORT_SYMBOL(kmem_cache_alloc);

kmem_cache_free_t kmem_cache_free;
EXPORT_SYMBOL(kmem_cache_free);

/*
 * Init
 */
start_kernel_t start_kernel_fn;
EXPORT_SYMBOL(start_kernel_fn);

void start_kernel(void)
{
    if (start_kernel_fn)
        start_kernel_fn();

    sbi_puts("\n##########################");
    sbi_puts("\nImpossible to come here!\n");
    sbi_puts("##########################\n");

    halt();
}
