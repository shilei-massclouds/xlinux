/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_TASK_STACK_H
#define _LINUX_SCHED_TASK_STACK_H

#ifndef __ASSEMBLY__

#include <sched.h>
static inline void *task_stack_page(const struct task_struct *task)
{
    return task->stack;
}

#endif /* __ASSEMBLY__ */

#endif /* _LINUX_SCHED_TASK_STACK_H */
