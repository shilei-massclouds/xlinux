/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _SBI_H
#define _SBI_H

#include <types.h>

void sbi_puts(const char *s);
void sbi_printf(const char *fmt, ...);

#endif /* _SBI_H */
