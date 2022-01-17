/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_FORK_H
#define _LINUX_FORK_H

#include <mm_types.h>

struct mm_struct *mm_alloc(void);

typedef struct vm_area_struct *
(*vm_area_alloc_t)(struct mm_struct *mm);
extern vm_area_alloc_t vm_area_alloc;

void set_mm_exe_file(struct mm_struct *mm, struct file *new_exe_file);

#endif /* _LINUX_FORK_H */
