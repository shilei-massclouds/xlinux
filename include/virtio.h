/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_VIRTIO_H
#define _LINUX_VIRTIO_H

#include <types.h>
#include <device.h>

struct virtio_device_id {
    u32 device;
    u32 vendor;
};

struct virtio_device {
    struct device dev;
    struct virtio_device_id id;
    struct list_head vqs;
    u64 features;
    void *priv;
};

#endif /* _LINUX_VIRTIO_H */
