// SPDX-License-Identifier: GPL-2.0+

#include <binfmts.h>

static int load_elf_binary(struct linux_binprm *bprm)
{
    panic("%s: !", __func__);
}

static struct linux_binfmt elf_format = {
    .load_binary = load_elf_binary,
};

int init_elf_binfmt(void)
{
    register_binfmt(&elf_format);
    return 0;
}

