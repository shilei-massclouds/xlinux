/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <fs.h>
#include <thread_info.h>

/* Used in tsk->state: */
#define TASK_UNINTERRUPTIBLE    0x0002

struct task_struct {
    struct thread_info thread_info;

    struct mm_struct *mm;
    struct mm_struct *active_mm;

    struct fs_struct *fs;

    /* Signal handlers: */
    struct signal_struct *signal;
};

#endif /* _LINUX_SCHED_H */
