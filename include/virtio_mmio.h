// SPDX-License-Identifier: GPL-2.0
#ifndef _VIRTIO_MMIO_H_
#define _VIRTIO_MMIO_H_

/* Magic value ("virt" string) - Read Only */
#define VIRTIO_MMIO_MAGIC_VALUE     0x000

/* Virtio device version - Read Only */
#define VIRTIO_MMIO_VERSION         0x004

/* Virtio device ID - Read Only */
#define VIRTIO_MMIO_DEVICE_ID       0x008

/* Virtio vendor ID - Read Only */
#define VIRTIO_MMIO_VENDOR_ID       0x00c

/* Guest's memory page size in bytes - Write Only */
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028


int virtio_mmio_init(void);

#endif /* _VIRTIO_MMIO_H_ */
