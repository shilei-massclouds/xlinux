/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <fs.h>
#include <thread_info.h>

/*
 * cloning flags:
 */
#define CSIGNAL     0x000000ff  /* signal mask to be sent at exit */
#define CLONE_VM    0x00000100  /* set if VM shared between processes */
#define CLONE_FS    0x00000200  /* set if fs info shared between processes */

#define CLONE_UNTRACED  0x00800000  /* set if the tracing process can't force CLONE_PTRACE on this clone */

/* Used in tsk->state: */
#define TASK_UNINTERRUPTIBLE    0x0002

#define PF_KTHREAD  0x00200000  /* I am a kernel thread */

extern unsigned long init_stack[THREAD_SIZE / sizeof(unsigned long)];

/* CPU-specific state of a task */
struct __riscv_d_ext_state {
    u64 f[32];
    u32 fcsr;
};

struct thread_struct {
    /* Callee-saved registers */
    unsigned long ra;
    unsigned long sp;   /* Kernel mode stack */
    unsigned long s[12];    /* s[0]: frame pointer */
    struct __riscv_d_ext_state fstate;
};

struct task_struct {
    struct thread_info thread_info;

    void *stack;

    /* Per task flags (PF_*), defined further below: */
    unsigned int flags;

    struct mm_struct *mm;
    struct mm_struct *active_mm;

    struct fs_struct *fs;

    /* Signal handlers: */
    struct signal_struct *signal;

    /* CPU-specific state of this task: */
    struct thread_struct thread;
};

typedef void (*schedule_tail_t)(struct task_struct *);
extern schedule_tail_t schedule_tail;

#endif /* _LINUX_SCHED_H */
