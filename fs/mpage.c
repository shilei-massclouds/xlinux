// SPDX-License-Identifier: GPL-2.0

#include <mm.h>
#include <fs.h>
#include <export.h>

int mpage_readpage(struct page *page, get_block_t get_block)
{
    /*
    struct mpage_readpage_args args = {
        .page = page,
        .nr_pages = 1,
        .get_block = get_block,
    };

    args.bio = do_mpage_readpage(&args);
    if (args.bio)
        mpage_bio_submit(REQ_OP_READ, 0, args.bio);
    return 0;
    */
    panic("%s: !", __func__);
}
EXPORT_SYMBOL(mpage_readpage);
