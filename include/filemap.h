/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_FILEMAP_H
#define _LINUX_FILEMAP_H

#include <fs.h>

struct page *
read_cache_page(struct address_space *mapping,
                pgoff_t index,
                int (*filler)(void *, struct page *),
                void *data);

int
add_to_page_cache_lru(struct page *page,
                      struct address_space *mapping,
                      pgoff_t offset,
                      gfp_t gfp_mask);

#endif /* _LINUX_FILEMAP_H */
