/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _PAGE_H
#define _PAGE_H

#include <config.h>
#include <const.h>

#define PAGE_SHIFT	(12)
#define PAGE_SIZE	(_AC(1, UL) << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))

#define PAGE_OFFSET _AC(CONFIG_PAGE_OFFSET, UL)

#define KERN_VIRT_SIZE (-PAGE_OFFSET)

#endif /* _PAGE_H */
