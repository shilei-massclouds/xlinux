/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BLK_TYPES_H
#define __LINUX_BLK_TYPES_H

/*
 * bio flags
 */
enum {
    BIO_NO_PAGE_REF,    /* don't put release vec pages */
    BIO_CLONED,         /* doesn't own data */
    BIO_BOUNCED,        /* bio is a bounce bio */
    BIO_USER_MAPPED,    /* contains user pages */
    BIO_NULL_MAPPED,    /* contains invalid user pages */
    BIO_WORKINGSET,     /* contains userspace workingset pages */
    BIO_QUIET,          /* Make BIO Quiet */
    BIO_CHAIN,          /* chained bio, ->bi_remaining in effect */
    BIO_REFFED,         /* bio has elevated ->bi_cnt */
    BIO_THROTTLED,      /* This bio has already been subjected to
                         * throttling rules. Don't do it again. */
    BIO_TRACE_COMPLETION,   /* bio_endio() should trace the final
                               completion of this bio. */
    BIO_CGROUP_ACCT,    /* has been accounted to a cgroup */
    BIO_TRACKED,        /* set if bio goes through the rq_qos path */
    BIO_FLAG_LAST
};

struct bio {
    struct bio *bi_next;    /* request queue link */

    struct gendisk *bi_disk;

    /* bottom bits req flags, top bits REQ_OP.
     * Use accessors. */
    unsigned int bi_opf;

    unsigned short bi_flags;    /* status, etc and bvec pool number */

    u8 bi_partno;
};

/* obsolete, don't use in new code */
static inline void
bio_set_op_attrs(struct bio *bio, unsigned op, unsigned op_flags)
{
    bio->bi_opf = op | op_flags;
}

#endif /* __LINUX_BLK_TYPES_H */
