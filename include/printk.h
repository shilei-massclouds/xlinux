/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _PRINTK_H_
#define _PRINTK_H_

#include <acgcc.h>
#include <types.h>

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

void printk(const char *fmt, ...);

#endif /* _PRINTK_H_ */
