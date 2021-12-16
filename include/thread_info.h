/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_RISCV_THREAD_INFO_H
#define _ASM_RISCV_THREAD_INFO_H

#include <page.h>

#define THREAD_SIZE_ORDER   (2)
#define THREAD_SIZE         (PAGE_SIZE << THREAD_SIZE_ORDER)

#define TASK_TI_FLAGS           0
#define TASK_TI_PREEMPT_COUNT   8
#define TASK_TI_KERNEL_SP       24
#define TASK_TI_USER_SP         32
#define TASK_TI_CPU             40

#ifndef __ASSEMBLY__

typedef struct {
    unsigned long seg;
} mm_segment_t;

struct thread_info {
    unsigned long   flags;          /* low level flags */
    int             preempt_count;  /* 0=>preemptible, <0=>BUG */
    mm_segment_t    addr_limit;
    /*
     * These stack pointers are overwritten on every system call or
     * exception.  SP is also saved to the stack it can be recovered when
     * overwritten.
     */
    long            kernel_sp;      /* Kernel stack pointer */
    long            user_sp;        /* User stack pointer */
    int             cpu;
};

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_RISCV_THREAD_INFO_H */
