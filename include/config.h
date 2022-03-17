/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _CONFIG_H
#define _CONFIG_H

#define CONFIG_HZ 250

/* Paging according to SV-39 */
#define CONFIG_VA_BITS      39
#define CONFIG_PA_BITS      56
#define CONFIG_PAGE_OFFSET  0xffffffe000000000
#define CONFIG_NR_CPUS      1

#define COMMAND_LINE_SIZE   512

#define CONFIG_DEFAULT_HOSTNAME "(none)"

#endif  /* _CONFIG_H */
