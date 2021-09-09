/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _SBI_H
#define _SBI_H

#include <types.h>

void sbi_console_puts(const char *s);
void sbi_console_printf(const char *fmt, ...);

#endif /* _SBI_H */
