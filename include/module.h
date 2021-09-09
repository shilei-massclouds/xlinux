/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_MODULE_H
#define _LINUX_MODULE_H

#include <export.h>
#include <list.h>

struct module {
    list_head list;

    const struct kernel_symbol *syms;
    unsigned int num_syms;
};

#endif /* _LINUX_MODULE_H */
