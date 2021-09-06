/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _SBI_H
#define _SBI_H

#include <types.h>

void sbi_console_puts(const char *s);
void hex_to_str(unsigned long num, char *str, size_t size);

#endif /* _SBI_H */
