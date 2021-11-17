/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <fs.h>

struct task_struct {
    struct fs_struct *fs;
};

#endif /* _LINUX_SCHED_H */
