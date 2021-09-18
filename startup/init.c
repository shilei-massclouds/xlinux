// SPDX-License-Identifier: GPL-2.0-only

#include <export.h>
#include <kernel.h>
#include <bug.h>

start_kernel_t start_kernel_fn;
EXPORT_SYMBOL(start_kernel_fn);

void start_kernel(void)
{
    sbi_printf("%s: ...\n", __func__);

    if (start_kernel_fn)
        start_kernel_fn();

    panic("Impossible to come here!");
}
