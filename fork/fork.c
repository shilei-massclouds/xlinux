// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <fork.h>
#include <slab.h>
#include <errno.h>
#include <sched.h>
#include <export.h>
#include <printk.h>
#include <string.h>
#include <current.h>
#include <cpumask.h>
#include <pgalloc.h>
#include <mm_types.h>
#include <user_namespace.h>

#define allocate_mm()   (kmem_cache_alloc(mm_cachep, GFP_KERNEL))

/* SLAB cache for vm_area_struct structures */
static struct kmem_cache *vm_area_cachep;

/* SLAB cache for mm_struct structures (tsk->mm) */
static struct kmem_cache *mm_cachep;

static inline int mm_alloc_pgd(struct mm_struct *mm)
{
    mm->pgd = pgd_alloc(mm);
    if (unlikely(!mm->pgd))
        return -ENOMEM;
    return 0;
}

static struct mm_struct *
mm_init(struct mm_struct *mm, struct task_struct *p)
{
    if (mm_alloc_pgd(mm))
        panic("bad memory!");

    return mm;
}

/*
 * Allocate and initialize an mm_struct.
 */
struct mm_struct *mm_alloc(void)
{
    struct mm_struct *mm;

    mm = allocate_mm();
    if (!mm)
        return NULL;

    memset(mm, 0, sizeof(*mm));
    return mm_init(mm, current);
}
EXPORT_SYMBOL(mm_alloc);

struct vm_area_struct *_vm_area_alloc(struct mm_struct *mm)
{
    struct vm_area_struct *vma;

    vma = kmem_cache_alloc(vm_area_cachep, GFP_KERNEL);
    if (vma)
        vma_init(vma, mm);
    return vma;
}

void set_mm_exe_file(struct mm_struct *mm, struct file *new_exe_file)
{
    mm->exe_file = new_exe_file;
}
EXPORT_SYMBOL(set_mm_exe_file);

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

    vm_area_cachep = KMEM_CACHE(vm_area_struct, SLAB_PANIC|SLAB_ACCOUNT);
}

static int
init_module(void)
{
    printk("module[fork]: init begin ...\n");

    proc_caches_init();

    vm_area_alloc = _vm_area_alloc;

    printk("module[fork]: init end!\n");

    return 0;
}
