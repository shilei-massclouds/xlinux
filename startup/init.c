// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <sbi.h>
#include <export.h>
#include <kernel.h>

start_kernel_t start_kernel_fn;
EXPORT_SYMBOL(start_kernel_fn);

void start_kernel(void)
{
    if (start_kernel_fn)
        start_kernel_fn();

    sbi_puts("\n##########################");
    sbi_puts("\nImpossible to come here!\n");
    sbi_puts("##########################\n");

    halt();
}
