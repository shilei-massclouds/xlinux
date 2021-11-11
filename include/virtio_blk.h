/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_VIRTIO_BLK_H
#define _LINUX_VIRTIO_BLK_H

/* Feature bits */
#define VIRTIO_BLK_F_SIZE_MAX   1   /* Indicates maximum segment size */
#define VIRTIO_BLK_F_SEG_MAX    2   /* Indicates maximum # of segments */
#define VIRTIO_BLK_F_GEOMETRY   4   /* Legacy geometry available  */
#define VIRTIO_BLK_F_RO         5   /* Disk is read-only */
#define VIRTIO_BLK_F_BLK_SIZE   6   /* Block size of disk is available*/
#define VIRTIO_BLK_F_TOPOLOGY   10  /* Topology information is available */
#define VIRTIO_BLK_F_MQ         12  /* support more than one vq */
#define VIRTIO_BLK_F_DISCARD    13  /* DISCARD is supported */
#define VIRTIO_BLK_F_WRITE_ZEROES   14  /* WRITE ZEROES is supported */

/* Legacy feature bits */
#define VIRTIO_BLK_F_BARRIER    0   /* Does host support barriers? */
#define VIRTIO_BLK_F_SCSI       7   /* Supports scsi command passthru */
#define VIRTIO_BLK_F_FLUSH      9   /* Flush command supported */
#define VIRTIO_BLK_F_CONFIG_WCE 11  /* Writeback mode available in config */

#endif /* _LINUX_VIRTIO_BLK_H */
