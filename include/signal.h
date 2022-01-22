/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_SIGNAL_H
#define _LINUX_SCHED_SIGNAL_H

#include <resource.h>

#define SIGILL      4

#define ILL_ILLTRP  4   /* illegal trap */

/*
 * SIGSEGV si_codes
 */
#define SEGV_MAPERR 1   /* address not mapped to object */
#define SEGV_ACCERR 2   /* invalid permissions for mapped object */

struct signal_struct {
    struct rlimit rlim[RLIM_NLIMITS];
};

#endif /* _LINUX_SCHED_SIGNAL_H */
