// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <fork.h>
#include <slab.h>
#include <errno.h>
#include <namei.h>
#include <export.h>
#include <kernel.h>
#include <limits.h>
#include <string.h>
#include <binfmts.h>
#include <current.h>
#include <resource.h>
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
    //vma->vm_flags = VM_STACK_FLAGS | VM_STACK_INCOMPLETE_SETUP;
    //vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);

    err = insert_vm_struct(mm, vma);
    if (err)
        panic("can not insert vma!");

    mm->stack_vm = mm->total_vm = 1;
    bprm->p = vma->vm_end - sizeof(void *);
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

    printk("%s: (%s)!\n", __func__, filename->name);

    retval = bprm_mm_init(bprm);
    if (retval)
        panic("can not init bprm mm!");
    return bprm;
}

static int count_strings_kernel(const char *const *argv)
{
    int i;

    if (!argv)
        return 0;

    for (i = 0; argv[i]; ++i) {
        if (i >= MAX_ARG_STRINGS)
            return -E2BIG;
    }
    return i;
}

static int bprm_stack_limits(struct linux_binprm *bprm)
{
    unsigned long limit, ptr_size;

    /*
     * Limit to 1/4 of the max stack size or 3/4 of _STK_LIM
     * (whichever is smaller) for the argv+env strings.
     * This ensures that:
     *  - the remaining binfmt code will not run out of stack space,
     *  - the program will have a reasonable amount of stack left
     *    to work from.
     */
    limit = _STK_LIM / 4 * 3;
    //limit = min(limit, bprm->rlim_stack.rlim_cur / 4);
    /*
     * We've historically supported up to 32 pages (ARG_MAX)
     * of argument strings even with small stacks
     */
    limit = max_t(unsigned long, limit, ARG_MAX);
    /*
     * We must account for the size of all the argv and envp pointers to
     * the argv and envp strings, since they will also take up space in
     * the stack. They aren't stored until much later when we can't
     * signal to the parent that the child has run out of stack space.
     * Instead, calculate it here so it's possible to fail gracefully.
     */
    ptr_size = (bprm->argc + bprm->envc) * sizeof(void *);
    if (limit <= ptr_size)
        return -E2BIG;
    limit -= ptr_size;

    bprm->argmin = bprm->p - limit;
    return 0;
}

static bool valid_arg_len(struct linux_binprm *bprm, long len)
{
    return len <= MAX_ARG_STRLEN;
}

/*
 * The nascent bprm->mm is not visible until exec_mmap() but it can
 * use a lot of memory, account these pages in current->mm temporary
 * for oom_badness()->get_mm_rss(). Once exec succeeds or fails, we
 * change the counter back via acct_arg_size(0).
 */
static void acct_arg_size(struct linux_binprm *bprm, unsigned long pages)
{
    struct mm_struct *mm = current->mm;
    long diff = (long)(pages - bprm->vma_pages);

    if (!mm || !diff)
        return;

    bprm->vma_pages = pages;
}

static struct page *
get_arg_page(struct linux_binprm *bprm, unsigned long pos, int write)
{
    int ret;
    struct page *page;
    unsigned int gup_flags = FOLL_FORCE;

    if (write)
        gup_flags |= FOLL_WRITE;

    /*
     * We are doing an exec().  'current' is the process
     * doing the exec and bprm->mm is the new process's mm.
     */
    ret = get_user_pages_remote(bprm->mm, pos, 1, gup_flags, &page,
                                NULL, NULL);
    if (ret <= 0)
        return NULL;

    if (write)
        acct_arg_size(bprm, vma_pages(bprm->vma));

    return page;
}

/*
 * Copy and argument/environment string from the kernel to the processes stack.
 */
int copy_string_kernel(const char *arg, struct linux_binprm *bprm)
{
    unsigned long pos = bprm->p;
    int len = strnlen(arg, MAX_ARG_STRLEN) + 1 /* terminating NUL */;

    if (len == 0)
        return -EFAULT;
    if (!valid_arg_len(bprm, len))
        return -E2BIG;

    /* We're going to work our way backwards. */
    arg += len;
    bprm->p -= len;
    if (bprm->p < bprm->argmin)
        return -E2BIG;

    while (len > 0) {
        char *kaddr;
        struct page *page;
        unsigned int bytes_to_copy =
            min_t(unsigned int, len,
                  min_not_zero(offset_in_page(pos), PAGE_SIZE));

        pos -= bytes_to_copy;
        arg -= bytes_to_copy;
        len -= bytes_to_copy;

        page = get_arg_page(bprm, pos, 1);
        if (!page)
            return -E2BIG;
        /*
        kaddr = kmap_atomic(page);
        flush_arg_page(bprm, pos & PAGE_MASK, page);
        memcpy(kaddr + offset_in_page(pos), arg, bytes_to_copy);
        flush_kernel_dcache_page(page);
        kunmap_atomic(kaddr);
        put_arg_page(page);
        */
        panic("%s: 1", __func__);
    }

    panic("%s: !", __func__);
    return 0;
}

static int
copy_strings_kernel(int argc, const char *const *argv,
                    struct linux_binprm *bprm)
{
    while (argc-- > 0) {
        int ret = copy_string_kernel(argv[argc], bprm);
        if (ret < 0)
            return ret;
    }
    return 0;
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
        panic("bad filename!");

    bprm = alloc_bprm(fd, filename);
    if (IS_ERR(bprm))
        panic("can not alloc bprm!");

    retval = count_strings_kernel(argv);
    if (retval < 0)
        panic("out of memory!");
    bprm->argc = retval;

    retval = count_strings_kernel(envp);
    if (retval < 0)
        panic("out of memory!");
    bprm->envc = retval;

    retval = bprm_stack_limits(bprm);
    if (retval < 0)
        panic("out of limits!");

    retval = copy_string_kernel(bprm->filename, bprm);
    if (retval < 0)
        panic("out of memory!");
    bprm->exec = bprm->p;

    retval = copy_strings_kernel(bprm->envc, envp, bprm);
    if (retval < 0)
        panic("out of memory!");

    retval = copy_strings_kernel(bprm->argc, argv, bprm);
    if (retval < 0)
        panic("out of memory!");

    panic("%s: kernel_filename(%s)!", __func__, kernel_filename);
}
EXPORT_SYMBOL(kernel_execve);
