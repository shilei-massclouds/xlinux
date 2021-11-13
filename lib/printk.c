// SPDX-License-Identifier: GPL-2.0-only
#include <sbi.h>
#include <acgcc.h>
#include <string.h>
#include <export.h>

#define PREFIX_MAX      32
#define LOG_LINE_MAX    (1024 - PREFIX_MAX)

static void
vprintk_func(const char *fmt, va_list args)
{
    static char textbuf[LOG_LINE_MAX];

    vsnprintf(textbuf, sizeof(textbuf), fmt, args);

    sbi_puts(textbuf);
}

void printk(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintk_func(fmt, args);
    va_end(args);
}
EXPORT_SYMBOL(printk);
