// SPDX-License-Identifier: GPL-2.0-only

#include <export.h>
#include <string.h>
#include <memblock.h>

void
kfree_const(const void *x)
{
}
EXPORT_SYMBOL(kfree_const);

char *
kstrdup(const char *s)
{
    size_t len;
    char *buf;

    if (!s)
        return NULL;

    len = strlen(s) + 1;
    buf = memblock_alloc(len, 8);
    if (buf)
        memcpy(buf, s, len);
    return buf;
}
EXPORT_SYMBOL(kstrdup);

const char *
kstrdup_const(const char *s)
{
    return kstrdup(s);
}
EXPORT_SYMBOL(kstrdup_const);
