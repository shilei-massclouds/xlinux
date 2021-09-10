// SPDX-License-Identifier: GPL-2.0-only
#include <export.h>
#include <sbi.h>

//const char info[] = "]\n";

static int init_module(void)
{
    sbi_puts("test_print: [\n");
}
//EXPORT_SYMBOL(test);
