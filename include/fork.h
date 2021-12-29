/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_FORK_H
#define _LINUX_FORK_H

#include <mm_types.h>

struct mm_struct *mm_alloc(void);

struct vm_area_struct *vm_area_alloc(struct mm_struct *mm);

#endif /* _LINUX_FORK_H */
