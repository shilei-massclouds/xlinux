// SPDX-License-Identifier: GPL-2.0-only

#include <export.h>
#include <printk.h>
#include <string.h>
#include <scatterlist.h>

/**
 * sg_init_table - Initialize SG table
 * @sgl:       The SG table
 * @nents:     Number of entries in table
 *
 * Notes:
 *   If this is part of a chained sg table, sg_mark_end() should be
 *   used only on the last table part.
 *
 **/
void sg_init_table(struct scatterlist *sgl, unsigned int nents)
{
    memset(sgl, 0, sizeof(*sgl) * nents);
    sg_init_marker(sgl, nents);
}
EXPORT_SYMBOL(sg_init_table);

static int
init_module(void)
{
    printk("module[scatterlist]: init begin ...\n");
    printk("module[scatterlist]: init end!\n");
    return 0;
}
