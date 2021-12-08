/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCATTERLIST_H
#define _LINUX_SCATTERLIST_H

#define SG_CHAIN    0x01UL
#define SG_END      0x02UL

struct scatterlist {
    unsigned long   page_link;
    /*
    unsigned int    offset;
    unsigned int    length;
    dma_addr_t      dma_address;
    */
};

void sg_init_table(struct scatterlist *sgl, unsigned int nents);

static inline void sg_mark_end(struct scatterlist *sg)
{
    /*
     * Set termination bit, clear potential chain bit
     */
    sg->page_link |= SG_END;
    sg->page_link &= ~SG_CHAIN;
}

static inline void
sg_init_marker(struct scatterlist *sgl, unsigned int nents)
{
    sg_mark_end(&sgl[nents - 1]);
}

#endif /* _LINUX_SCATTERLIST_H */
