// SPDX-License-Identifier: GPL-2.0-only

#include <bio.h>
#include <bvec.h>
#include <slab.h>
#include <errno.h>
#include <printk.h>
#include <export.h>

/*
 * Test patch to inline a certain number of bi_io_vec's inside the bio
 * itself, to shrink a bio data allocation from two mempool calls to one
 */
#define BIO_INLINE_VECS     4

/*
 * fs_bio_set is the bio_set containing bio and
 * iovec memory pools used by IO code
 * that does not need private memory pools.
 */
struct bio_set fs_bio_set;
EXPORT_SYMBOL(fs_bio_set);

struct bio_slab {
    struct kmem_cache *slab;
    /*
    unsigned int slab_ref;
    unsigned int slab_size;
    char name[8];
    */
};

static struct bio_slab *bio_slabs;
static unsigned int bio_slab_nr, bio_slab_max;

struct bio *
bio_alloc_bioset(gfp_t gfp_mask,
                 unsigned int nr_iovecs,
                 struct bio_set *bs)
{
    panic("%s: bs(0x%p) nr_iovecs(%u)", __func__, bs, nr_iovecs);
}
EXPORT_SYMBOL(bio_alloc_bioset);

static struct kmem_cache *
bio_find_or_create_slab(unsigned int extra_size)
{
    panic("%s: !", __func__);
}

int
bioset_init(struct bio_set *bs,
            unsigned int pool_size,
            unsigned int front_pad,
            int flags)
{
    unsigned int back_pad = BIO_INLINE_VECS * sizeof(struct bio_vec);

    bs->bio_slab = bio_find_or_create_slab(front_pad + back_pad);
    if (!bs->bio_slab)
        return -ENOMEM;

    if (mempool_init_slab_pool(&bs->bio_pool, pool_size, bs->bio_slab))
        panic("bad slab pool!");

    return 0;
}

static int init_bio(void)
{
    bio_slab_max = 2;
    bio_slabs = kcalloc(bio_slab_max, sizeof(struct bio_slab), GFP_KERNEL);

    if (bioset_init(&fs_bio_set, BIO_POOL_SIZE, 0, BIOSET_NEED_BVECS))
        panic("bio: can't allocate bios\n");

    return 0;
}

static int
init_module(void)
{
    printk("module[bio]: init begin ...\n");
    init_bio();
    printk("module[bio]: init end!\n");
    return 0;
}
