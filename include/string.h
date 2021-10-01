/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_STRING_H
#define _LINUX_STRING_H

#include <acgcc.h>
#include <types.h>

extern void *memset(void *, int, __kernel_size_t);
extern void *memcpy(void *, const void *, size_t);
extern int memcmp(const void *cs, const void *ct, size_t count);
extern void *memchr(const void *s, int c, size_t n);
extern void *memmove(void *dest, const void *src, size_t count);
extern int strcmp(const char *cs, const char *ct);
extern size_t strlen(const char *s);
extern size_t strcspn(const char *s, const char *reject);

extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s,int c);
extern char *strchrnul(const char *s, int c);

extern int strncmp(const char *cs, const char *ct, size_t count);

extern size_t strnlen(const char *s, size_t count);
extern int strcasecmp(const char *s1, const char *s2);

extern char *strreplace(char *s, char old, char new);

static inline unsigned char
__tolower(unsigned char c)
{
    if (isupper(c))
        c -= 'A'-'a';
    return c;
}

static inline unsigned char
__toupper(unsigned char c)
{
    if (islower(c))
        c -= 'a'-'A';
    return c;
}

#define tolower(c) __tolower(c)
#define toupper(c) __toupper(c)

/**
 * kbasename - return the last part of a pathname.
 *
 * @path: path to extract the filename from.
 */
static inline const char *
kbasename(const char *path)
{
    const char *tail = strrchr(path, '/');
    return tail ? tail + 1 : path;
}

char *kvasprintf(const char *fmt, va_list ap);

const char *
kvasprintf_const(const char *fmt, va_list ap);

#endif /* _LINUX_STRING_H */