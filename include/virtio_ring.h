// SPDX-License-Identifier: GPL-2.0+

#ifndef _UAPI_LINUX_VIRTIO_RING_H
#define _UAPI_LINUX_VIRTIO_RING_H

/* We support indirect buffer descriptors */
#define VIRTIO_RING_F_INDIRECT_DESC 28

/* The Guest publishes the used index for which it expects an interrupt
 * at the end of the avail ring. Host should ignore the avail->flags field. */
/* The Host publishes the avail index for which it expects a kick
 * at the end of the used ring. Guest should ignore the used->flags field. */
#define VIRTIO_RING_F_EVENT_IDX     29

/*
 * If clear - device has the platform DMA (e.g. IOMMU) bypass quirk feature.
 * If set - use platform DMA tools to access the memory.
 *
 * Note the reverse polarity (compared to most other features),
 * this is for compatibility with legacy systems.
 */
#define VIRTIO_F_ACCESS_PLATFORM    33

/* This feature indicates support for the packed virtqueue layout. */
#define VIRTIO_F_RING_PACKED        34

/*
 * This feature indicates that memory accesses by the driver and the
 * device are ordered in a way described by the platform.
 */
#define VIRTIO_F_ORDER_PLATFORM     36

void vring_transport_features(struct virtio_device *vdev);

#endif /* _UAPI_LINUX_VIRTIO_RING_H */
