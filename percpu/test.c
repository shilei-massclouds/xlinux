// SPDX-License-Identifier: GPL-2.0-only
#include <percpu.h>
#include <printk.h>

static int
init_module(void)
{
    void *p;

    printk("test percpu ...\n");
    p = __alloc_percpu(16, 8);
    return 0;
}
