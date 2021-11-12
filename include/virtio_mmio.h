// SPDX-License-Identifier: GPL-2.0
#ifndef _VIRTIO_MMIO_H_
#define _VIRTIO_MMIO_H_

/* Magic value ("virt" string) - Read Only */
#define VIRTIO_MMIO_MAGIC_VALUE         0x000

/* Virtio device version - Read Only */
#define VIRTIO_MMIO_VERSION             0x004

/* Virtio device ID - Read Only */
#define VIRTIO_MMIO_DEVICE_ID           0x008

/* Virtio vendor ID - Read Only */
#define VIRTIO_MMIO_VENDOR_ID           0x00c

/* Bitmask of the features supported by the device (host)
 * (32 bits per set) - Read Only */
#define VIRTIO_MMIO_DEVICE_FEATURES     0x010

/* Device (host) features set selector - Write Only */
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014

/* Bitmask of features activated by the driver (guest)
 * (32 bits per set) - Write Only */
#define VIRTIO_MMIO_DRIVER_FEATURES     0x020

/* Activated features set selector - Write Only */
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024

/* Guest's memory page size in bytes - Write Only */
#define VIRTIO_MMIO_GUEST_PAGE_SIZE     0x028

/* Device status register - Read Write */
#define VIRTIO_MMIO_STATUS              0x070

/* The config space is defined by each driver as
 * the per-driver configuration space - Read Write */
#define VIRTIO_MMIO_CONFIG              0x100


int virtio_mmio_init(void);

#endif /* _VIRTIO_MMIO_H_ */
