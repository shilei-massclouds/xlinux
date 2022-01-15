// SPDX-License-Identifier: GPL-2.0+

#include <fs.h>
#include <elf.h>
#include <mman.h>
#include <slab.h>
#include <errno.h>
#include <string.h>
#include <binfmts.h>
#include <processor.h>
#include <mman-common.h>

#define ELF_MIN_ALIGN       PAGE_SIZE
#define ELF_PAGEOFFSET(_v)  ((_v) & (ELF_MIN_ALIGN-1))
#define ELF_PAGEALIGN(_v)   (((_v) + ELF_MIN_ALIGN - 1) & ~(ELF_MIN_ALIGN - 1))

#define BAD_ADDR(x) (unlikely((unsigned long)(x) >= TASK_SIZE))

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

static inline int
make_prot(u32 p_flags)
{
    int prot = 0;

    if (p_flags & PF_R)
        prot |= PROT_READ;
    if (p_flags & PF_W)
        prot |= PROT_WRITE;
    if (p_flags & PF_X)
        prot |= PROT_EXEC;

    return prot;
}

static unsigned long
elf_map(struct file *filep, unsigned long addr,
        const struct elf_phdr *eppnt, int prot, int type,
        unsigned long total_size)
{
    /*
    unsigned long map_addr;
    unsigned long size = eppnt->p_filesz + ELF_PAGEOFFSET(eppnt->p_vaddr);
    unsigned long off = eppnt->p_offset - ELF_PAGEOFFSET(eppnt->p_vaddr);

    size = ELF_PAGEALIGN(size);

    if (!size)
        return addr;

    if (total_size)
        panic("total size is NOT zero!");
    else
        map_addr = vm_mmap(filep, addr, size, prot, type, off);
        */
    panic("%s: !", __func__);
}

static int load_elf_binary(struct linux_binprm *bprm)
{
    int i;
    int retval;
    unsigned long error;
    struct elf_phdr *elf_ppnt;
    struct elf_phdr *elf_phdata;
    unsigned long elf_bss, elf_brk;
    unsigned long start_code, end_code, start_data, end_data;
    int load_addr_set = 0;
    unsigned long load_addr = 0, load_bias = 0;
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

    /* Do this so that we can load the interpreter, if need be.  We will
       change some of these later */
    retval = setup_arg_pages(bprm, STACK_TOP, executable_stack);
    if (retval < 0)
        panic("setup arg pages error!");

    elf_bss = 0;
    elf_brk = 0;

    start_code = ~0UL;
    end_code = 0;
    start_data = 0;
    end_data = 0;

    /* Now we do a little grungy work by mmapping the ELF image into
       the correct location in memory. */
    for(i = 0, elf_ppnt = elf_phdata;
        i < elf_ex->e_phnum; i++, elf_ppnt++) {
        int elf_prot, elf_flags;
        unsigned long k, vaddr;
        unsigned long total_size = 0;

        if (elf_ppnt->p_type != PT_LOAD)
            continue;

        if (unlikely (elf_brk > elf_bss))
            panic("bad elf_brk or elf_bss!");

        elf_prot = make_prot(elf_ppnt->p_flags);

        elf_flags = MAP_PRIVATE | MAP_DENYWRITE | MAP_EXECUTABLE;

        vaddr = elf_ppnt->p_vaddr;

        if (elf_ex->e_type == ET_EXEC || load_addr_set) {
            elf_flags |= MAP_FIXED;
        } else if (elf_ex->e_type == ET_DYN) {
            panic("bad e_type ET_DYN!");
        }

        /*
        error = elf_map(bprm->file, load_bias + vaddr, elf_ppnt,
                        elf_prot, elf_flags, total_size);
        if (BAD_ADDR(error))
            panic("elf map error!");
            */

        panic("%s: 1", __func__);
    }

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

