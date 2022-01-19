// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <sbi.h>
#include <fork.h>
#include <slab.h>
#include <sched.h>
#include <export.h>
#include <kernel.h>
#include <ptrace.h>
#include <signal.h>
#include <filemap.h>

void (*handle_arch_irq)(struct pt_regs *);
EXPORT_SYMBOL(handle_arch_irq);

/* to be mentioned only in INIT_TASK */
struct fs_struct init_fs = {
};

static struct signal_struct init_signals = {
    .rlim   = INIT_RLIMITS,
};

struct task_struct init_task
__aligned(L1_CACHE_BYTES) = {
    .thread_info = INIT_THREAD_INFO(init_task),

    .stack  = init_stack,
    .flags  = PF_KTHREAD,
    .fs     = &init_fs,
    .signal = &init_signals,

    .normal_prio = MAX_PRIO - 20,
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

/* For fork */
vm_area_alloc_t vm_area_alloc;
EXPORT_SYMBOL(vm_area_alloc);

/* For specific cache allocation */
kmem_cache_alloc_t kmem_cache_alloc;
EXPORT_SYMBOL(kmem_cache_alloc);

kmem_cache_free_t kmem_cache_free;
EXPORT_SYMBOL(kmem_cache_free);

/* For block device */
struct super_block *blockdev_superblock;
EXPORT_SYMBOL(blockdev_superblock);

I_BDEV_T I_BDEV;
EXPORT_SYMBOL(I_BDEV);

/* For filemap */
add_to_page_cache_lru_t add_to_page_cache_lru;
EXPORT_SYMBOL(add_to_page_cache_lru);

/* For sched */
schedule_tail_t schedule_tail;
EXPORT_SYMBOL(schedule_tail);

/*
 * Init
 */
start_kernel_t start_kernel_fn;
EXPORT_SYMBOL(start_kernel_fn);

extern void ret_from_kernel_thread(void);
EXPORT_SYMBOL(ret_from_kernel_thread);

void start_kernel(void)
{
    if (start_kernel_fn)
        start_kernel_fn();

    sbi_puts("\n##########################");
    sbi_puts("\nImpossible to come here!\n");
    sbi_puts("##########################\n");

    halt();
}
