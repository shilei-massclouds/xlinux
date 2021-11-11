/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _LINUX_VIRTIO_CONFIG_H
#define _LINUX_VIRTIO_CONFIG_H

#include <virtio.h>
#include <interrupt.h>

typedef void vq_callback_t(struct virtqueue *);
struct virtio_config_ops {
    void (*get)(struct virtio_device *vdev, unsigned offset,
                void *buf, unsigned len);
    void (*set)(struct virtio_device *vdev, unsigned offset,
                const void *buf, unsigned len);
    u32 (*generation)(struct virtio_device *vdev);
    u8 (*get_status)(struct virtio_device *vdev);
    void (*set_status)(struct virtio_device *vdev, u8 status);
    void (*reset)(struct virtio_device *vdev);
    int (*find_vqs)(struct virtio_device *, unsigned nvqs,
                    struct virtqueue *vqs[], vq_callback_t *callbacks[],
                    const char * const names[], const bool *ctx,
                    struct irq_affinity *desc);
    void (*del_vqs)(struct virtio_device *);
    u64 (*get_features)(struct virtio_device *vdev);
    int (*finalize_features)(struct virtio_device *vdev);
    const char *(*bus_name)(struct virtio_device *vdev);
    int (*set_vq_affinity)(struct virtqueue *vq,
                           const struct cpumask *cpu_mask);
    const struct cpumask *(*get_vq_affinity)(struct virtio_device *vdev,
                                             int index);
};

#endif /* _LINUX_VIRTIO_CONFIG_H */
