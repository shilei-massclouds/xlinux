// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
#include <slab.h>
#include <errno.h>
#include <export.h>
#include <pgtable.h>
#include <vmalloc.h>

//static struct kmem_cache *vmap_area_cachep;

static bool vmap_initialized;

/*
static struct vmap_area *
alloc_vmap_area(unsigned long size, unsigned long align,
                unsigned long vstart, unsigned long vend,
                gfp_t gfp_mask)
{
    struct vmap_area *va;

    BUG_ON(!size);
    BUG_ON(offset_in_page(size));
    BUG_ON(!is_power_of_2(align));

    if (unlikely(!vmap_initialized))
        return ERR_PTR(-EBUSY);

    gfp_mask = gfp_mask & GFP_RECLAIM_MASK;

    va = kmem_cache_alloc_node(vmap_area_cachep, gfp_mask);
    if (unlikely(!va))
        return ERR_PTR(-ENOMEM);

}

static inline void
setup_vmalloc_vm_locked(struct vm_struct *vm,
                        struct vmap_area *va,
                        unsigned long flags,
                        const void *caller)
{
    vm->flags = flags;
    vm->addr = (void *)va->va_start;
    vm->size = va->va_end - va->va_start;
    vm->caller = caller;
    va->vm = vm;
}

static void
setup_vmalloc_vm(struct vm_struct *vm, struct vmap_area *va,
                 unsigned long flags, const void *caller)
{
    setup_vmalloc_vm_locked(vm, va, flags, caller);
}

static struct vm_struct *
__get_vm_area_node(unsigned long size, unsigned long align, unsigned long flags,
                   unsigned long start, unsigned long end,
                   gfp_t gfp_mask, const void *caller)
{
    struct vmap_area *va;
    struct vm_struct *area;
    unsigned long requested_size = size;

    size = PAGE_ALIGN(size);
    if (unlikely(!size))
        return NULL;

    if (flags & VM_IOREMAP)
        align = 1ul << clamp_t(int, get_count_order_long(size),
                               PAGE_SHIFT, IOREMAP_MAX_ORDER);

    area = kzalloc_node(sizeof(*area), gfp_mask & GFP_RECLAIM_MASK);
    if (unlikely(!area))
        return NULL;

    if (!(flags & VM_NO_GUARD))
        size += PAGE_SIZE;

    va = alloc_vmap_area(size, align, start, end, gfp_mask);
    if (IS_ERR(va)) {
        kfree(area);
        return NULL;
    }

    setup_vmalloc_vm(area, va, flags, caller);
    return area;
}

void *
__vmalloc_node_range(unsigned long size, unsigned long align,
                     unsigned long start, unsigned long end, gfp_t gfp_mask,
                     pgprot_t prot, unsigned long vm_flags,
                     const void *caller)
{
    void *addr;
    struct vm_struct *area;
    unsigned long real_size = size;

    size = PAGE_ALIGN(size);
    if (!size || (size >> PAGE_SHIFT) > totalram_pages())
        goto fail;

    area = __get_vm_area_node(real_size, align,
                              VM_ALLOC | VM_UNINITIALIZED | vm_flags,
                              start, end, gfp_mask, caller);
    if (!area)
        goto fail;

    return addr;

 fail:
    return NULL;
}

void *
__vmalloc_node(unsigned long size, unsigned long align,
               gfp_t gfp_mask, const void *caller)
{
    return __vmalloc_node_range(size, align, VMALLOC_START, VMALLOC_END,
                                gfp_mask, PAGE_KERNEL, 0, caller);
}
*/

/**
 * vmalloc - allocate virtually contiguous memory
 * @size:    allocation size
 *
 * Allocate enough pages to cover @size from the page level
 * allocator and map them into contiguous kernel virtual space.
 *
 * For tight control over page level allocator and protection flags
 * use __vmalloc() instead.
 *
 * Return: pointer to the allocated memory or %NULL on error
 */
/*
void *
vmalloc(unsigned long size)
{
    return __vmalloc_node(size, 1, GFP_KERNEL, __builtin_return_address(0));
}
EXPORT_SYMBOL(vmalloc);
*/

static int
init_module(void)
{
    printk("module[vmalloc]: init begin ...\n");

    /*
     * Create the cache for vmap_area objects.
     */
    //vmap_area_cachep = KMEM_CACHE(vmap_area, SLAB_PANIC);

    printk("module[vmalloc]: init end!\n");
    return 0;
}
