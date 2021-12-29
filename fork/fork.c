// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <slab.h>
#include <export.h>
#include <printk.h>
#include <cpumask.h>
#include <mm_types.h>

#define allocate_mm()   (kmem_cache_alloc(mm_cachep, GFP_KERNEL))

/* SLAB cache for mm_struct structures (tsk->mm) */
static struct kmem_cache *mm_cachep;

/*
 * Allocate and initialize an mm_struct.
 */
struct mm_struct *mm_alloc(void)
{
    struct mm_struct *mm;

    mm = allocate_mm();
    if (!mm)
        return NULL;

    panic("%s: !", __func__);
    /*
    memset(mm, 0, sizeof(*mm));
    return mm_init(mm, current, current_user_ns());
    */
}
EXPORT_SYMBOL(mm_alloc);

#define ARCH_MIN_MMSTRUCT_ALIGN 0

void proc_caches_init(void)
{
    unsigned int mm_size;

    /*
     * The mm_cpumask is located at the end of mm_struct, and is
     * dynamically sized based on the maximum CPU number this system
     * can have, taking hotplug into account (nr_cpu_ids).
     */
    mm_size = sizeof(struct mm_struct) + cpumask_size();

    mm_cachep =
        kmem_cache_create_usercopy("mm_struct", mm_size,
                                   ARCH_MIN_MMSTRUCT_ALIGN,
                                   SLAB_HWCACHE_ALIGN|SLAB_PANIC|SLAB_ACCOUNT,
                                   offsetof(struct mm_struct, saved_auxv),
                                   sizeof_field(struct mm_struct, saved_auxv),
                                   NULL);
}

static int
init_module(void)
{
    printk("module[fork]: init begin ...\n");

    proc_caches_init();

    printk("module[fork]: init end!\n");

    return 0;
}
