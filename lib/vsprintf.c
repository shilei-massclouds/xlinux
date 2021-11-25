// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <page.h>
#include <acgcc.h>
#include <errno.h>
#include <types.h>
#include <export.h>
#include <kernel.h>
#include <string.h>

#define SIGN    1       /* unsigned/signed, must be 1 */
#define LEFT    2       /* left justified */
#define PLUS    4       /* show plus */
#define SPACE   8       /* space if plus */
#define ZEROPAD 16      /* pad with zero, must be 16 == '0' - ' ' */
#define SMALL   32      /* use lowercase in hex (must be 32 == 0x20) */
#define SPECIAL 64      /* prefix hex with "0x", octal with "0" */

struct printf_spec {
    unsigned int    type:8;         /* format_type enum */
    signed int      field_width:24; /* width of output field */
    unsigned int    flags:8;        /* flags to number() */
    unsigned int    base:8;         /* number base, 8, 10 or 16 only */
    signed int      precision:16;   /* # of digits/chars */
} __packed;

enum format_type {
    FORMAT_TYPE_NONE, /* Just a string part */
    FORMAT_TYPE_WIDTH,
    FORMAT_TYPE_PRECISION,
    FORMAT_TYPE_CHAR,
    FORMAT_TYPE_STR,
    FORMAT_TYPE_PTR,
    FORMAT_TYPE_PERCENT_CHAR,
    FORMAT_TYPE_INVALID,
    FORMAT_TYPE_LONG_LONG,
    FORMAT_TYPE_ULONG,
    FORMAT_TYPE_LONG,
    FORMAT_TYPE_UBYTE,
    FORMAT_TYPE_BYTE,
    FORMAT_TYPE_USHORT,
    FORMAT_TYPE_SHORT,
    FORMAT_TYPE_UINT,
    FORMAT_TYPE_INT,
    FORMAT_TYPE_SIZE_T,
    FORMAT_TYPE_PTRDIFF
};

const char hex_asc_upper[] = "0123456789ABCDEF";

static const char *
check_pointer_msg(const void *ptr)
{
    if (!ptr)
        return "(null)";

    if ((unsigned long)ptr < PAGE_SIZE || IS_ERR_VALUE(ptr))
        return "(efault)";

    return NULL;
}

static char *
string_nocheck(char *buf, char *end, const char *s, struct printf_spec spec)
{
    int len = 0;
    int lim = spec.precision;

    while (lim--) {
        char c = *s++;
        if (!c)
            break;
        if (buf < end)
            *buf = c;
        ++buf;
        ++len;
    }
    return buf;
}

static int
check_pointer(char **buf, char *end, const void *ptr, struct printf_spec spec)
{
    const char *err_msg;

    err_msg = check_pointer_msg(ptr);
    if (err_msg) {
        *buf = string_nocheck(*buf, end, err_msg, spec);
        return -EFAULT;
    }

    return 0;
}

/* Handle string from a well known address. */
static char *
string(char *buf, char *end, const char *s, struct printf_spec spec)
{
    if (check_pointer(&buf, end, s, spec))
        return buf;

    return string_nocheck(buf, end, s, spec);
}

static char *
number(char *buf, char *end, unsigned long long num, struct printf_spec spec)
{
    int i = 0;
    char tmp[3 * sizeof(num)] __aligned(2);

    if (spec.base == 16 || spec.base == 8) {
        int mask = spec.base - 1;
        int shift = (spec.base == 16) ? 4 : 3;

        do {
            tmp[i++] = hex_asc_upper[((unsigned char)num) & mask];
            num >>= shift;
        } while (num);
    } else if (spec.base == 10) {
        if (spec.flags & SIGN) {
            if ((signed long long)num < 0) {
                num = -(signed long long)num;
                if (buf < end)
                    *buf = '-';
                ++buf;
            }
        }

        do {
            tmp[i++] = hex_asc_upper[num % 10];
            num /= 10;
        } while (num);
    } else {
        BUG_ON(true);
    }

    /* actual digits of result */
    while (--i >= 0) {
        if (buf < end)
            *buf = tmp[i];
        ++buf;
    }

    return buf;
}

static int
format_decode(const char *fmt, struct printf_spec *spec)
{
    char qualifier;
    const char *start = fmt;

    /* By default */
    spec->type = FORMAT_TYPE_NONE;

    for (; *fmt; ++fmt) {
        if (*fmt == '%')
            break;
    }

    /* Return the current non-format string */
    if (fmt != start || !*fmt)
        return fmt - start;

    /* skip '%' */
    fmt++;

    /* get the precision */
    spec->precision = -1;

    /* get the conversion qualifier */
    qualifier = 0;
    if (*fmt == 'l')
        qualifier = *fmt++;

    /* default base */
    spec->base = 10;
    switch (*fmt) {
    case 'c':
        spec->type = FORMAT_TYPE_CHAR;
        return ++fmt - start;
    case 's':
        spec->type = FORMAT_TYPE_STR;
        return ++fmt - start;
    case 'o':
        spec->base = 8;
        break;
    case 'X':
    case 'x':
        spec->base = 16;
        break;
    case 'd':
    case 'i':
        spec->flags |= SIGN;
    case 'u':
        break;
    default:
        BUG_ON(true);
        spec->type = FORMAT_TYPE_INVALID;
        return fmt - start;
    }

    if (qualifier == 'l') {
        spec->type = FORMAT_TYPE_ULONG;
    } else {
        spec->type = FORMAT_TYPE_UINT + (spec->flags & SIGN);
    }

    return ++fmt - start;
}

int
vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    char *str;
    char *end;
    unsigned long long num;
    struct printf_spec spec = {0};

    str = buf;
    end = str + size;

    while (*fmt) {
        const char *old_fmt = fmt;
        int read = format_decode(fmt, &spec);

        fmt += read;
        switch (spec.type) {
        case FORMAT_TYPE_NONE: {
            int copy = read;
            if (str < end) {
                if (copy > end - str)
                    copy = end - str;
                memcpy(str, old_fmt, copy);
            }
            str += read;
            break;
        }
        case FORMAT_TYPE_CHAR: {
            char c;
            c = (unsigned char) va_arg(args, int);
            if (str < end)
                *str = c;
            ++str;
            break;
        }
        case FORMAT_TYPE_STR:
            str = string(str, end, va_arg(args, char *), spec);
            break;
        default:
            switch (spec.type) {
            case FORMAT_TYPE_ULONG:
                num = va_arg(args, unsigned long);
                break;
            case FORMAT_TYPE_LONG:
                num = va_arg(args, long);
                break;
            case FORMAT_TYPE_INT:
                num = (int) va_arg(args, int);
                break;
            case FORMAT_TYPE_UINT:
                num = va_arg(args, unsigned int);
                break;
            default:
                BUG_ON(true);
            }

            str = number(str, end, num, spec);
        }
    }

    if (size > 0) {
        if (str < end)
            *str = '\0';
        else
            end[-1] = '\0';
    }

    return str - buf;
}
EXPORT_SYMBOL(vsnprintf);

int
snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsnprintf(buf, size, fmt, args);
    va_end(args);

    return i;
}
EXPORT_SYMBOL(snprintf);

/**
 * sprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @fmt: The format string to use
 * @...: Arguments for the format string
 *
 * The function returns the number of characters written
 * into @buf. Use snprintf() or scnprintf() in order to avoid
 * buffer overflows.
 *
 * See the vsnprintf() documentation for format string extensions over C99.
 */
int
sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsnprintf(buf, INT_MAX, fmt, args);
    va_end(args);

    return i;
}
EXPORT_SYMBOL(sprintf);
