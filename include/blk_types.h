/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BLK_TYPES_H
#define __LINUX_BLK_TYPES_H

#define REQ_OP_BITS 8
#define REQ_OP_MASK ((1 << REQ_OP_BITS) - 1)

enum req_flag_bits {
    __REQ_FAILFAST_DEV =    /* no driver retries of device errors */
        REQ_OP_BITS,
    __REQ_FAILFAST_TRANSPORT, /* no driver retries of transport errors */
    __REQ_FAILFAST_DRIVER,  /* no driver retries of driver errors */
    __REQ_SYNC,     /* request is sync (sync write or read) */
    __REQ_META,     /* metadata io request */
    __REQ_PRIO,     /* boost priority in cfq */
    __REQ_NOMERGE,      /* don't touch this for merging */
    __REQ_IDLE,     /* anticipate more IO after this one */
    __REQ_INTEGRITY,    /* I/O includes block integrity payload */
    __REQ_FUA,      /* forced unit access */
    __REQ_PREFLUSH,     /* request for cache flush */
    __REQ_RAHEAD,       /* read ahead, can fail anytime */
    __REQ_BACKGROUND,   /* background IO */
    __REQ_NOWAIT,           /* Don't wait if request will block */
    /*
     * When a shared kthread needs to issue a bio for a cgroup, doing
     * so synchronously can lead to priority inversions as the kthread
     * can be trapped waiting for that cgroup.  CGROUP_PUNT flag makes
     * submit_bio() punt the actual issuing to a dedicated per-blkcg
     * work item to avoid such priority inversions.
     */
    __REQ_CGROUP_PUNT,

    /* command specific flags for REQ_OP_WRITE_ZEROES: */
    __REQ_NOUNMAP,      /* do not free blocks when zeroing */

    __REQ_HIPRI,

    /* for driver use */
    __REQ_DRV,
    __REQ_SWAP,     /* swapping request. */
    __REQ_NR_BITS,      /* stops here */
};

#define REQ_HIPRI       (1ULL << __REQ_HIPRI)

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
