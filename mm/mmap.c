// SPDX-License-Identifier: GPL-2.0-only

#include <mm.h>
#include <errno.h>
#include <export.h>
#include <rbtree.h>

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
