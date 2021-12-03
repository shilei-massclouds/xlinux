/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BLK_TYPES_H
#define __LINUX_BLK_TYPES_H

struct bio {
    struct bio *bi_next;    /* request queue link */
};

#endif /* __LINUX_BLK_TYPES_H */
