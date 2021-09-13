// SPDX-License-Identifier: GPL-2.0-only
#include <export.h>
#include <types.h>
#include <bug.h>
#include <page.h>
#include <kernel.h>
#include <errno.h>

#define PREFIX_MAX      32
#define LOG_LINE_MAX    (1024 - PREFIX_MAX)

typedef __builtin_va_list   va_list;
#define va_start(v, l)      __builtin_va_start(v, l)
#define va_end(v)           __builtin_va_end(v)
#define va_arg(v, l)        __builtin_va_arg(v, l)
#define va_copy(d, s)       __builtin_va_copy(d, s)

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

enum sbi_ext_id {
    SBI_EXT_0_1_SET_TIMER = 0x0,
    SBI_EXT_0_1_CONSOLE_PUTCHAR = 0x1,
    SBI_EXT_0_1_CONSOLE_GETCHAR = 0x2,
    SBI_EXT_0_1_CLEAR_IPI = 0x3,
    SBI_EXT_0_1_SEND_IPI = 0x4,
    SBI_EXT_0_1_REMOTE_FENCE_I = 0x5,
    SBI_EXT_0_1_REMOTE_SFENCE_VMA = 0x6,
    SBI_EXT_0_1_REMOTE_SFENCE_VMA_ASID = 0x7,
    SBI_EXT_0_1_SHUTDOWN = 0x8,
};

struct sbiret {
    long error;
    long value;
};

const char hex_asc_upper[] = "0123456789ABCDEF";

struct sbiret
sbi_ecall(int ext, int fid,
          unsigned long arg0, unsigned long arg1, unsigned long arg2,
          unsigned long arg3, unsigned long arg4, unsigned long arg5)
{
    struct sbiret ret;

    register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);
    register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);
    register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);
    register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);
    register uintptr_t a4 asm ("a4") = (uintptr_t)(arg4);
    register uintptr_t a5 asm ("a5") = (uintptr_t)(arg5);
    register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);
    register uintptr_t a7 asm ("a7") = (uintptr_t)(ext);
    asm volatile ("ecall"
                  : "+r" (a0), "+r" (a1)
                  : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
                  : "memory");
    ret.error = a0;
    ret.value = a1;

    return ret;
}
EXPORT_SYMBOL(sbi_ecall);

void sbi_putchar(int ch)
{
    sbi_ecall(SBI_EXT_0_1_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0, 0, 0);
}
EXPORT_SYMBOL(sbi_putchar);

void sbi_puts(const char *s)
{
    for (; *s; s++) {
        if (*s == '\n')
            sbi_putchar('\r');
        sbi_putchar(*s);
    }
}
EXPORT_SYMBOL(sbi_puts);

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
    case 's':
        spec->type = FORMAT_TYPE_STR;
        return ++fmt - start;
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

static const char *
check_pointer_msg(const void *ptr)
{
    if (!ptr)
        return "(null)";

    if ((unsigned long)ptr < PAGE_SIZE || IS_ERR_VALUE(ptr))
        return "(efault)";

    return NULL;
}

/* Handle string from a well known address. */
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

    if (spec.base == 16) {
        do {
            tmp[i++] = hex_asc_upper[((unsigned char)num) & 0xF];
            num >>= 4;
        } while (num);

        *buf = '0';
        ++buf;

        *buf = 'X';
        ++buf;
    } else if (spec.base == 10) {
        do {
            tmp[i++] = hex_asc_upper[num % 10];
            num /= 10;
        } while (num);

        if (spec.flags & SIGN) {
            *buf = '-';
            ++buf;
        }
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

static void
vprintk_func(const char *fmt, va_list args)
{
    char *str;
    char *end;
    unsigned long long num;
    static char textbuf[LOG_LINE_MAX];
    struct printf_spec spec = {0};

    str = textbuf;
    end = str + sizeof(textbuf);

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
                num = -(signed long long)num;
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

    if (str < end)
        *str = '\0';
    else
        end[-1] = '\0';

    sbi_puts(textbuf);
}

void sbi_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintk_func(fmt, args);
    va_end(args);
}
EXPORT_SYMBOL(sbi_printf);
