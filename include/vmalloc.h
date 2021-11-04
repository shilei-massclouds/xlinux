// SPDX-License-Identifier: GPL-2.0-only
#ifndef _VMALLOC_H_
#define _VMALLOC_H_

#include <gfp.h>

#define VM_IOREMAP          0x00000001  /* ioremap() and friends */
#define VM_ALLOC            0x00000002  /* vmalloc() */
#define VM_UNINITIALIZED    0x00000020  /* vm_struct is not fully initialized */
#define VM_NO_GUARD         0x00000040  /* don't add guard page */

#define IOREMAP_MAX_ORDER   (7 + PAGE_SHIFT)    /* 128 pages */

struct vm_struct {
    void            *addr;
    unsigned long   size;
    unsigned long   flags;
    const void      *caller;
};

struct vmap_area {
    unsigned long va_start;
    unsigned long va_end;

    union {
        struct vm_struct *vm;           /* in "busy" tree */
    };
};

void *vmalloc(unsigned long size);

#endif /* _VMALLOC_H_ */