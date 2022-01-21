/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_SIGNAL_H
#define _LINUX_SCHED_SIGNAL_H

#include <resource.h>

#define SIGILL      4

#define ILL_ILLTRP  4   /* illegal trap */

struct signal_struct {
    struct rlimit rlim[RLIM_NLIMITS];
};

#endif /* _LINUX_SCHED_SIGNAL_H */
