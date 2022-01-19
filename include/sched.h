/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <fs.h>
#include <thread_info.h>

#define MAX_NICE    19
#define MIN_NICE    -20
#define NICE_WIDTH  (MAX_NICE - MIN_NICE + 1)

#define MAX_USER_RT_PRIO    100
#define MAX_RT_PRIO MAX_USER_RT_PRIO

#define MAX_PRIO    (MAX_RT_PRIO + NICE_WIDTH)

/*
 * cloning flags:
 */
#define CSIGNAL     0x000000ff  /* signal mask to be sent at exit */
#define CLONE_VM    0x00000100  /* set if VM shared between processes */
#define CLONE_FS    0x00000200  /* set if fs info shared between processes */

#define CLONE_UNTRACED  0x00800000  /* set if the tracing process can't force CLONE_PTRACE on this clone */

/* Used in tsk->state: */
#define TASK_RUNNING            0x0000
#define TASK_INTERRUPTIBLE      0x0001
#define TASK_UNINTERRUPTIBLE    0x0002

#define TASK_NEW    0x0800

#define PF_KTHREAD  0x00200000  /* I am a kernel thread */

#define cpu_rq()    (&runqueue)
#define task_rq(p)  cpu_rq()

#define TASK_ON_RQ_QUEUED   1

#define ENQUEUE_NOCLOCK     0x08

struct task_struct;

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

struct rq {
};

struct sched_class {
    void (*enqueue_task)(struct rq *rq, struct task_struct *p, int flags);
};

struct task_struct {
    struct thread_info thread_info;

    /* -1 unrunnable, 0 runnable, >0 stopped: */
    volatile long state;

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

    const struct sched_class *sched_class;

    int prio;
    int normal_prio;
    int on_rq;
};

static struct rq runqueue;

typedef void (*schedule_tail_t)(struct task_struct *);
extern schedule_tail_t schedule_tail;

void wake_up_new_task(struct task_struct *p);

int sched_fork(unsigned long clone_flags, struct task_struct *p);

#endif /* _LINUX_SCHED_H */
