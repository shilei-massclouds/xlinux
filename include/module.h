/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_MODULE_H
#define _LINUX_MODULE_H

#include <export.h>

struct module {
    const struct kernel_symbol *syms;
    unsigned int num_syms;
};

#endif /* _LINUX_MODULE_H */
