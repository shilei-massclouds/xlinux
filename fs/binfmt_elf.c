// SPDX-License-Identifier: GPL-2.0+

#include <fs.h>
#include <elf.h>
#include <slab.h>
#include <errno.h>
#include <string.h>
#include <binfmts.h>

#define ELF_MIN_ALIGN   PAGE_SIZE

static int
elf_read(struct file *file, void *buf, size_t len, loff_t pos)
{
    ssize_t rv;

    rv = kernel_read(file, buf, len, &pos);
    if (unlikely(rv != len)) {
        return (rv < 0) ? rv : -EIO;
    }
    return 0;
}

static struct elf_phdr *
load_elf_phdrs(const struct elfhdr *elf_ex, struct file *elf_file)
{
    int retval;
    unsigned int size;
    struct elf_phdr *elf_phdata = NULL;

    /*
     * If the size of this structure has changed, then punt, since
     * we will be doing the wrong thing.
     */
    if (elf_ex->e_phentsize != sizeof(struct elf_phdr))
        panic("bad phdr!");

    /* Sanity check the number of program headers... */
    /* ...and their total size. */
    size = sizeof(struct elf_phdr) * elf_ex->e_phnum;
    if (size == 0 || size > 65536 || size > ELF_MIN_ALIGN)
        panic("bad size!");

    elf_phdata = kmalloc(size, GFP_KERNEL);
    if (!elf_phdata)
        panic("out of memory!");

    /* Read in the program headers */
    retval = elf_read(elf_file, elf_phdata, size, elf_ex->e_phoff);
    if (retval < 0)
        panic("read elf header error!");

    return elf_phdata;
}

static int load_elf_binary(struct linux_binprm *bprm)
{
    int i;
    int retval;
    struct elf_phdr *elf_ppnt;
    struct elf_phdr *elf_phdata;
    int executable_stack = EXSTACK_DEFAULT;
    struct elf_phdr *elf_property_phdata = NULL;
    struct elfhdr *elf_ex = (struct elfhdr *)bprm->buf;
    struct file *interpreter = NULL; /* to shut gcc up */

    retval = -ENOEXEC;
    /* First of all, some simple consistency checks */
    if (memcmp(elf_ex->e_ident, ELFMAG, SELFMAG) != 0)
        goto out;

    if (elf_ex->e_type != ET_EXEC && elf_ex->e_type != ET_DYN)
        goto out;
    if (!elf_check_arch(elf_ex))
        goto out;
    if (!bprm->file->f_op->mmap)
        goto out;

    elf_phdata = load_elf_phdrs(elf_ex, bprm->file);
    if (!elf_phdata)
        goto out;

    elf_ppnt = elf_phdata;
    for (i = 0; i < elf_ex->e_phnum; i++, elf_ppnt++) {
        char *elf_interpreter;

        if (elf_ppnt->p_type == PT_GNU_PROPERTY) {
            elf_property_phdata = elf_ppnt;
            continue;
        }

        if (elf_ppnt->p_type != PT_INTERP)
            continue;

        panic("is interp!");
    }

    elf_ppnt = elf_phdata;
    for (i = 0; i < elf_ex->e_phnum; i++, elf_ppnt++) {
        switch (elf_ppnt->p_type) {
        case PT_GNU_STACK:
            if (elf_ppnt->p_flags & PF_X)
                executable_stack = EXSTACK_ENABLE_X;
            else
                executable_stack = EXSTACK_DISABLE_X;
            break;

        case PT_LOPROC ... PT_HIPROC:
            panic("bad p_type!");
            break;
        }
    }

    if (interpreter)
        panic("is interpreter!");

    retval = begin_new_exec(bprm);
    if (retval)
        panic("begin new exec error!");

    setup_new_exec(bprm);

    panic("%s: !", __func__);

    retval = 0;

 out:
    panic("%s: error!", __func__);
    return retval;
}

static struct linux_binfmt elf_format = {
    .load_binary = load_elf_binary,
};

int init_elf_binfmt(void)
{
    register_binfmt(&elf_format);
    return 0;
}

