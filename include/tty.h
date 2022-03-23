/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TTY_H
#define _LINUX_TTY_H

struct tty_port {
    unsigned char   console:1,      /* port is a console */
                    low_latency:1;  /* optional: tune for latency */
};

#endif /* _LINUX_TTY_H */
