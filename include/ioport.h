/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IOPORT_H
#define _LINUX_IOPORT_H

#include <types.h>

#define IORESOURCE_TYPE_BITS    0x00001f00  /* Resource type */
#define IORESOURCE_IO           0x00000100  /* PCI/ISA I/O ports */
#define IORESOURCE_MEM          0x00000200

/*
 * Resources are tree-like, allowing
 * nesting etc..
 */
struct resource {
    resource_size_t start;
    resource_size_t end;
    const char *name;
    unsigned long flags;
    unsigned long desc;
    struct resource *parent, *sibling, *child;
};

static inline unsigned long
resource_type(const struct resource *res)
{
    return res->flags & IORESOURCE_TYPE_BITS;
}

#endif  /* _LINUX_IOPORT_H */
