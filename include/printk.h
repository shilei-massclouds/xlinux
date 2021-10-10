/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _PRINTK_H_
#define _PRINTK_H_

#include <acgcc.h>
#include <types.h>

#define _CP_RESET   "\033[0m"
#define _CP_RED     "\033[31;1m"
#define _CP_GREEN   "\033[32;1m"
#define _CP_YELLOW  "\033[33;1m"
#define _CP_BLUE    "\033[34;1m"

#define _COLORED(text, color) _CP_RESET color text _CP_RESET
#define _RED(text)      _COLORED(text, _CP_RED)
#define _GREEN(text)    _COLORED(text, _CP_GREEN)
#define _YELLOW(text)   _COLORED(text, _CP_YELLOW)
#define _BLUE(text)     _COLORED(text, _CP_BLUE)

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

void printk(const char *fmt, ...);

#endif /* _PRINTK_H_ */
