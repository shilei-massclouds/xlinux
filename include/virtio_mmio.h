// SPDX-License-Identifier: GPL-2.0
#ifndef _VIRTIO_MMIO_H_
#define _VIRTIO_MMIO_H_

/* Magic value ("virt" string) - Read Only */
#define VIRTIO_MMIO_MAGIC_VALUE     0x000

int virtio_mmio_init(void);

#endif /* _VIRTIO_MMIO_H_ */
