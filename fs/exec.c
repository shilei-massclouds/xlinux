// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <fork.h>
#include <slab.h>
#include <errno.h>
#include <namei.h>
#include <export.h>
#include <string.h>
#include <binfmts.h>
#include <processor.h>

static int __bprm_mm_init(struct linux_binprm *bprm)
{
    int err;
    struct mm_struct *mm = bprm->mm;
    struct vm_area_struct *vma = NULL;

    bprm->vma = vma = vm_area_alloc(mm);
    if (!vma)
        return -ENOMEM;
    vma_set_anonymous(vma);

    vma->vm_end = STACK_TOP_MAX;
    vma->vm_start = vma->vm_end - PAGE_SIZE;
    //vma->vm_flags = VM_SOFTDIRTY | VM_STACK_FLAGS | VM_STACK_INCOMPLETE_SETUP;
    //vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);

    err = insert_vm_struct(mm, vma);
    if (err)
        panic("can not insert vma!");

    mm->stack_vm = mm->total_vm = 1;
    bprm->p = vma->vm_end - sizeof(void *);
    panic("%s: !", __func__);
    return 0;
}

/*
 * Create a new mm_struct and populate it with a temporary stack
 * vm_area_struct.  We don't have enough context at this point to
 * set the stack flags, permissions, and offset, so we use
 * temporary values.  We'll update them later in setup_arg_pages().
 */
static int bprm_mm_init(struct linux_binprm *bprm)
{
    int err;
    struct mm_struct *mm = NULL;

    bprm->mm = mm = mm_alloc();
    err = -ENOMEM;
    if (!mm)
        panic("no memory!");

    err = __bprm_mm_init(bprm);
    if (err)
        panic("bad mm for bprm!");

    panic("%s: !", __func__);
    return 0;
}

static struct linux_binprm *alloc_bprm(int fd, struct filename *filename)
{
    struct linux_binprm *bprm = kzalloc(sizeof(*bprm), GFP_KERNEL);
    int retval = -ENOMEM;
    if (!bprm)
        return ERR_PTR(retval);

    if (fd == AT_FDCWD || filename->name[0] == '/') {
        bprm->filename = filename->name;
    } else {
        if (filename->name[0] == '\0')
            bprm->fdpath = kasprintf(GFP_KERNEL, "/dev/fd/%d", fd);
        else
            bprm->fdpath = kasprintf(GFP_KERNEL, "/dev/fd/%d/%s",
                                     fd, filename->name);
        if (!bprm->fdpath)
            return ERR_PTR(retval);

        bprm->filename = bprm->fdpath;
    }
    bprm->interp = bprm->filename;

    retval = bprm_mm_init(bprm);
    if (retval)
        panic("can not init bprm mm!");
    panic("%s: (%s)!", __func__, filename->name);
    return bprm;
}

int kernel_execve(const char *kernel_filename,
                  const char *const *argv, const char *const *envp)
{
    int retval;
    struct filename *filename;
    struct linux_binprm *bprm;
    int fd = AT_FDCWD;

    filename = getname_kernel(kernel_filename);
    if (IS_ERR(filename))
        return PTR_ERR(filename);

    bprm = alloc_bprm(fd, filename);
    if (IS_ERR(bprm)) {
        retval = PTR_ERR(bprm);
        return retval;
    }

    panic("%s: kernel_filename(%s)!", __func__, kernel_filename);
}
EXPORT_SYMBOL(kernel_execve);
