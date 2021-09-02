/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_EXPORT_H
#define _LINUX_EXPORT_H

#include <compiler_attributes.h>

#define __KSYMTAB_ENTRY(sym)                               \
    static const struct kernel_symbol __ksymtab_##sym           \
    __attribute__((section("___ksymtab+" #sym), used))   \
    __aligned(sizeof(void *))                                   \
    = { (unsigned long)&sym, __kstrtab_##sym }

struct kernel_symbol {
    unsigned long value;
    const char *name;
};

#define EXPORT_SYMBOL(sym)                                          \
    extern typeof(sym) sym;                                         \
    extern const char __kstrtab_##sym[];                            \
    asm("   .section \"__ksymtab_strings\",\"aMS\",%progbits,1  \n" \
        "__kstrtab_" #sym ":                    \n"                 \
        "   .asciz  \"" #sym "\"                \n"                 \
        "   .previous                           \n");               \
    __KSYMTAB_ENTRY(sym)

#endif /* _LINUX_EXPORT_H */
