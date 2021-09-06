// SPDX-License-Identifier: GPL-2.0-only
#include <export.h>
#include <sbi.h>

//const char info[] = "]\n";

void test_print(unsigned long num)
{
    //char str[64];
    sbi_console_puts("test_print: [\n");
    //hex_to_str(num, str, sizeof(str));
    //sbi_console_puts(str);
    //sbi_console_puts(info);
}
EXPORT_SYMBOL(test_print);
