/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MM_TYPES_H
#define _LINUX_MM_TYPES_H

#define page_private(page)  ((page)->private)

static inline void
set_page_private(struct page *page, unsigned long private)
{
    page->private = private;
}

#endif /* _LINUX_MM_TYPES_H */
