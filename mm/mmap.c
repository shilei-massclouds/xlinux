// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
#include <mman.h>
#include <stat.h>
#include <errno.h>
#include <export.h>
#include <limits.h>
#include <rbtree.h>
#include <current.h>

/* enforced gap between the expanding stack and other mappings. */
unsigned long stack_guard_gap = 256UL<<PAGE_SHIFT;
EXPORT_SYMBOL(stack_guard_gap);

static int
find_vma_links(struct mm_struct *mm, unsigned long addr,
               unsigned long end, struct vm_area_struct **pprev,
               struct rb_node ***rb_link, struct rb_node **rb_parent)
{
    struct rb_node **__rb_link, *__rb_parent, *rb_prev;

    __rb_link = &mm->mm_rb.rb_node;
    rb_prev = __rb_parent = NULL;

    while (*__rb_link) {
        struct vm_area_struct *vma_tmp;

        __rb_parent = *__rb_link;
        vma_tmp = rb_entry(__rb_parent, struct vm_area_struct, vm_rb);

        if (vma_tmp->vm_end > addr) {
            /* Fail if an existing vma overlaps the area */
            if (vma_tmp->vm_start < end)
                return -ENOMEM;
            __rb_link = &__rb_parent->rb_left;
        } else {
            rb_prev = __rb_parent;
            __rb_link = &__rb_parent->rb_right;
        }
    }

    *pprev = NULL;
    if (rb_prev)
        *pprev = rb_entry(rb_prev, struct vm_area_struct, vm_rb);
    *rb_link = __rb_link;
    *rb_parent = __rb_parent;
    return 0;
}

static inline unsigned long vma_compute_gap(struct vm_area_struct *vma)
{
    unsigned long gap, prev_end;

    /*
     * Note: in the rare case of a VM_GROWSDOWN above a VM_GROWSUP, we
     * allow two stack_guard_gaps between them here, and when choosing
     * an unmapped area; whereas when expanding we only require one.
     * That's a little inconsistent, but keeps the code here simpler.
     */
    gap = vm_start_gap(vma);
    if (vma->vm_prev) {
        prev_end = vm_end_gap(vma->vm_prev);
        if (gap > prev_end)
            gap -= prev_end;
        else
            gap = 0;
    }
    return gap;
}

RB_DECLARE_CALLBACKS_MAX(static, vma_gap_callbacks,
                         struct vm_area_struct, vm_rb,
                         unsigned long, rb_subtree_gap, vma_compute_gap)

/*
 * Update augmented rbtree rb_subtree_gap values after vma->vm_start or
 * vma->vm_prev->vm_end values changed, without modifying the vma's position
 * in the rbtree.
 */
static void vma_gap_update(struct vm_area_struct *vma)
{
    /*
     * As it turns out, RB_DECLARE_CALLBACKS_MAX() already created
     * a callback function that does exactly what we want.
     */
    vma_gap_callbacks_propagate(&vma->vm_rb, NULL);
}

static inline void
vma_rb_insert(struct vm_area_struct *vma, struct rb_root *root)
{
    /* All rb_subtree_gap values must be consistent prior to insertion */
    rb_insert_augmented(&vma->vm_rb, root, &vma_gap_callbacks);
}

void __vma_link_rb(struct mm_struct *mm, struct vm_area_struct *vma,
        struct rb_node **rb_link, struct rb_node *rb_parent)
{
    /* Update tracking information for the gap following the new vma. */
    if (vma->vm_next)
        vma_gap_update(vma->vm_next);
    else
        mm->highest_vm_end = vm_end_gap(vma);

    /*
     * vma->vm_prev wasn't known when we followed the rbtree to find the
     * correct insertion point for that vma. As a result, we could not
     * update the vma vm_rb parents rb_subtree_gap values on the way down.
     * So, we first insert the vma with a zero rb_subtree_gap value
     * (to be consistent with what we did on the way down), and then
     * immediately update the gap to the correct value. Finally we
     * rebalance the rbtree after all augmented values have been set.
     */
    rb_link_node(&vma->vm_rb, rb_parent, rb_link);
    vma->rb_subtree_gap = 0;
    vma_gap_update(vma);
    vma_rb_insert(vma, &mm->mm_rb);
}

static void
__vma_link(struct mm_struct *mm, struct vm_area_struct *vma,
    struct vm_area_struct *prev, struct rb_node **rb_link,
    struct rb_node *rb_parent)
{
    __vma_link_list(mm, vma, prev);
    __vma_link_rb(mm, vma, rb_link, rb_parent);
}

static void
vma_link(struct mm_struct *mm, struct vm_area_struct *vma,
         struct vm_area_struct *prev, struct rb_node **rb_link,
         struct rb_node *rb_parent)
{
    __vma_link(mm, vma, prev, rb_link, rb_parent);
}

/* Insert vm structure into process list sorted by address
 * and into the inode's i_mmap tree.  If vm_file is non-NULL
 * then i_mmap_rwsem is taken here.
 */
int insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vma)
{
    struct vm_area_struct *prev;
    struct rb_node **rb_link, *rb_parent;

    if (find_vma_links(mm, vma->vm_start, vma->vm_end,
                       &prev, &rb_link, &rb_parent))
        return -ENOMEM;

    if (vma_is_anonymous(vma)) {
        vma->vm_pgoff = vma->vm_start >> PAGE_SHIFT;
    }

    vma_link(mm, vma, prev, rb_link, rb_parent);
    return 0;
}
EXPORT_SYMBOL(insert_vm_struct);

/* Look up the first VMA which satisfies  addr < vm_end,  NULL if none. */
struct vm_area_struct *
find_vma(struct mm_struct *mm, unsigned long addr)
{
    struct rb_node *rb_node;
    struct vm_area_struct *vma;

    rb_node = mm->mm_rb.rb_node;

    while (rb_node) {
        struct vm_area_struct *tmp;

        tmp = rb_entry(rb_node, struct vm_area_struct, vm_rb);

        if (tmp->vm_end > addr) {
            vma = tmp;
            if (tmp->vm_start <= addr)
                break;
            rb_node = rb_node->rb_left;
        } else
            rb_node = rb_node->rb_right;
    }

    return vma;
}
EXPORT_SYMBOL(find_vma);

struct vm_area_struct *
find_extend_vma(struct mm_struct *mm, unsigned long addr)
{
    unsigned long start;
    struct vm_area_struct *vma;

    addr &= PAGE_MASK;
    vma = find_vma(mm, addr);
    if (!vma)
        return NULL;
    if (vma->vm_start <= addr)
        return vma;

    panic("%s: (%lx <= %lx)!",
          __func__, vma->vm_start, addr);
}
EXPORT_SYMBOL(find_extend_vma);

pgprot_t protection_map[16] = {
    __P000, __P001, __P010, __P011, __P100, __P101, __P110, __P111,
    __S000, __S001, __S010, __S011, __S100, __S101, __S110, __S111
};

static inline pgprot_t arch_filter_pgprot(pgprot_t prot)
{
    return prot;
}

pgprot_t vm_get_page_prot(unsigned long vm_flags)
{
    pgprot_t ret =
        __pgprot(pgprot_val(protection_map[vm_flags &
                            (VM_READ|VM_WRITE|VM_EXEC|VM_SHARED)]) |
                 pgprot_val(arch_vm_get_page_prot(vm_flags)));

    return arch_filter_pgprot(ret);
}
EXPORT_SYMBOL(vm_get_page_prot);

int expand_downwards(struct vm_area_struct *vma,
                     unsigned long address)
{
    struct vm_area_struct *prev;

    /* Enforce stack_guard_gap */
    prev = vma->vm_prev;

    /* We must make sure the anon_vma is allocated. */
    /*
    if (unlikely(anon_vma_prepare(vma)))
        panic("out of memory!");
        */

    if (address < vma->vm_start) {
        unsigned long size, grow;

        size = vma->vm_end - address;
        grow = (vma->vm_start - address) >> PAGE_SHIFT;

        if (grow <= vma->vm_pgoff) {
            vma->vm_start = address;
            vma->vm_pgoff -= grow;
            vma_gap_update(vma);
        }
    }

    return 0;
}

int expand_stack(struct vm_area_struct *vma, unsigned long address)
{
    return expand_downwards(vma, address);
}
EXPORT_SYMBOL(expand_stack);

unsigned long
get_unmapped_area(struct file *file, unsigned long addr, unsigned long len,
                  unsigned long pgoff, unsigned long flags)
{
    unsigned long (*get_area)(struct file *, unsigned long,
                              unsigned long, unsigned long, unsigned long);

    /* Careful about overflows.. */
    if (len > TASK_SIZE)
        return -ENOMEM;

    get_area = current->mm->get_unmapped_area;

    addr = get_area(file, addr, len, pgoff, flags);
    if (IS_ERR_VALUE(addr))
        panic("bad addr!");

    if (addr > TASK_SIZE - len)
        panic("out of memory!");
    if (offset_in_page(addr))
        panic("bad arg!");

    return addr;
}

static inline u64
file_mmap_size_max(struct file *file, struct inode *inode)
{
    if (S_ISREG(inode->i_mode))
        return MAX_LFS_FILESIZE;

    if (S_ISBLK(inode->i_mode))
        return MAX_LFS_FILESIZE;

    if (S_ISSOCK(inode->i_mode))
        return MAX_LFS_FILESIZE;

    /* Special "we do even unsigned file positions" case */
    if (file->f_mode & FMODE_UNSIGNED_OFFSET)
        return 0;

    /* Yes, random drivers might want more. But I'm tired of buggy drivers */
    return ULONG_MAX;
}

static inline bool
file_mmap_ok(struct file *file, struct inode *inode,
             unsigned long pgoff, unsigned long len)
{
    u64 maxsize = file_mmap_size_max(file, inode);

    if (maxsize && len > maxsize)
        return false;
    maxsize -= len;
    if (pgoff > maxsize >> PAGE_SHIFT)
        return false;
    return true;
}

unsigned long
mmap_region(struct file *file, unsigned long addr,
            unsigned long len, vm_flags_t vm_flags, unsigned long pgoff,
            struct list_head *uf)
{
    panic("%s: !", __func__);
}

unsigned long
do_mmap(struct file *file, unsigned long addr,
        unsigned long len, unsigned long prot,
        unsigned long flags, unsigned long pgoff,
        unsigned long *populate, struct list_head *uf)
{
    vm_flags_t vm_flags;
    struct mm_struct *mm = current->mm;

    *populate = 0;

    if (!len)
        return -EINVAL;

    /* Careful about overflows.. */
    len = PAGE_ALIGN(len);
    if (!len)
        return -ENOMEM;

    /* offset overflow? */
    if ((pgoff + (len >> PAGE_SHIFT)) < pgoff)
        return -EOVERFLOW;

    /* Obtain the address to map to. we verify (or select) it and ensure
     * that it represents a valid section of the address space.
     */
    addr = get_unmapped_area(file, addr, len, pgoff, flags);
    if (IS_ERR_VALUE(addr))
        panic("get unmapped area!");

    /* Do simple checking here so the lower-level routines won't have
     * to. we assume access permissions have been handled by the open
     * of the memory object, so we don't do any here.
     */
    vm_flags = calc_vm_prot_bits(prot) | calc_vm_flag_bits(flags) |
        mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC;

    if (file) {
        unsigned long flags_mask;
        struct inode *inode = file_inode(file);

        if (!file_mmap_ok(file, inode, pgoff, len))
            panic("overflow!");

        //flags_mask = LEGACY_MAP_MASK | file->f_op->mmap_supported_flags;
        flags_mask = LEGACY_MAP_MASK;

        switch (flags & MAP_TYPE) {
            case MAP_SHARED:
                panic("MAP_SHARED!");
            case MAP_SHARED_VALIDATE:
                panic("MAP_SHARED_VALIDATE!");
            case MAP_PRIVATE:
                if (!(file->f_mode & FMODE_READ))
                    return -EACCES;

                if (!file->f_op->mmap)
                    return -ENODEV;
                if (vm_flags & (VM_GROWSDOWN|VM_GROWSUP))
                    return -EINVAL;

                break;
            default:
                panic("bad flags!");
        }
    } else {
        panic("no file!");
    }

    if (flags & MAP_NORESERVE)
        panic("MAP_NORESERVE!");

    addr = mmap_region(file, addr, len, vm_flags, pgoff, uf);
    if (!IS_ERR_VALUE(addr) &&
        ((vm_flags & VM_LOCKED) ||
         (flags & (MAP_POPULATE | MAP_NONBLOCK)) == MAP_POPULATE))
        panic("set poplate!");

    panic("%s: !", __func__);
}
EXPORT_SYMBOL(do_mmap);
