// SPDX-License-Identifier: GPL-2.0
#include <mm.h>
#include <bug.h>
#include <export.h>
#include <string.h>
#include <printk.h>
#include <memblock.h>

/* Simplified asprintf. */
char *
kvasprintf(const char *fmt, va_list ap)
{
    unsigned int first, second;
    char *p;
    va_list aq;

    va_copy(aq, ap);
    first = vsnprintf(NULL, 0, fmt, aq);
    va_end(aq);

    p = memblock_alloc(first+1, 8);
    if (!p)
        return NULL;

    second = vsnprintf(p, first+1, fmt, ap);

    BUG_ON(first != second);

    return p;
}
EXPORT_SYMBOL(kvasprintf);

const char *
kvasprintf_const(const char *fmt, va_list ap)
{
    if (!strchr(fmt, '%'))
        return kstrdup_const(fmt);
    if (!strcmp(fmt, "%s"))
        return kstrdup_const(va_arg(ap, const char*));
    return kvasprintf(fmt, ap);
}
EXPORT_SYMBOL(kvasprintf_const);
