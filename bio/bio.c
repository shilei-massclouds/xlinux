// SPDX-License-Identifier: GPL-2.0-only

#include <bio.h>
#include <bvec.h>
#include <slab.h>
#include <errno.h>
#include <printk.h>
#include <string.h>
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
    unsigned int slab_size;
    char name[8];
};

static struct bio_slab *bio_slabs;
static unsigned int bio_slab_nr, bio_slab_max;

void bio_init(struct bio *bio, struct bio_vec *table,
              unsigned short max_vecs)
{
    memset(bio, 0, sizeof(*bio));

    /*
    bio->bi_io_vec = table;
    bio->bi_max_vecs = max_vecs;
    */
}
EXPORT_SYMBOL(bio_init);

struct bio *
bio_alloc_bioset(gfp_t gfp_mask,
                 unsigned int nr_iovecs,
                 struct bio_set *bs)
{
    void *p;
    struct bio *bio;
    unsigned front_pad;
    unsigned inline_vecs;

    printk("%s: bs(0x%p) nr_iovecs(%u)\n", __func__, bs, nr_iovecs);

    if (!bs) {
        panic("bs is NULL!");
    } else {
        p = kmem_cache_alloc(bs->bio_slab, gfp_mask);
        front_pad = bs->front_pad;
        inline_vecs = BIO_INLINE_VECS;
    }

    if (unlikely(!p))
        return NULL;

    bio = p + front_pad;
    bio_init(bio, NULL, 0);
    return bio;
}
EXPORT_SYMBOL(bio_alloc_bioset);

int
bioset_init(struct bio_set *bs,
            unsigned int pool_size,
            unsigned int front_pad,
            int flags)
{
    unsigned int back_pad = BIO_INLINE_VECS * sizeof(struct bio_vec);
    unsigned int extra_size = front_pad + back_pad;
    unsigned int sz = sizeof(struct bio) + extra_size;

    bs->front_pad = front_pad;

    bs->bio_slab = kmem_cache_create("bio-0", sz, ARCH_KMALLOC_MINALIGN,
                                     SLAB_HWCACHE_ALIGN, NULL);
    if (!bs->bio_slab)
        return -ENOMEM;

    return 0;
}

static int init_bio(void)
{
    bio_slab_max = 2;
    bio_slab_nr = 0;
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
