/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_VIRTIO_H
#define _LINUX_VIRTIO_H

#include <types.h>
#include <device.h>
#include <driver.h>
#include <virtio_config.h>

struct virtio_device_id {
    u32 device;
    u32 vendor;
};
#define VIRTIO_DEV_ANY_ID   0xffffffff

struct virtio_device {
    int index;
    struct device dev;
    bool config_enabled;
    bool config_change_pending;
    struct virtio_device_id id;
    const struct virtio_config_ops *config;
    struct list_head vqs;
    u64 features;
    void *priv;
};

/**
 * virtqueue - a queue to register buffers for sending or receiving.
 * @list: the chain of virtqueues for this device
 * @callback: the function to call when buffers are consumed (can be NULL).
 * @name: the name of this virtqueue (mainly for debugging)
 * @vdev: the virtio device this queue was created for.
 * @priv: a pointer for the virtqueue implementation to use.
 * @index: the zero-based ordinal number for this queue.
 * @num_free: number of elements we expect to be able to fit.
 *
 * A note on @num_free: with indirect buffers, each buffer needs one
 * element in the queue, otherwise a buffer will need one element per
 * sg element.
 */
struct virtqueue {
    struct list_head list;
    void (*callback)(struct virtqueue *vq);
    const char *name;
    struct virtio_device *vdev;
    unsigned int index;
    unsigned int num_free;
    void *priv;
};

struct virtio_driver {
    struct device_driver driver;
    const struct virtio_device_id *id_table;
    /*
    const unsigned int *feature_table;
    unsigned int feature_table_size;
    const unsigned int *feature_table_legacy;
    unsigned int feature_table_size_legacy;
    int (*validate)(struct virtio_device *dev);
    int (*probe)(struct virtio_device *dev);
    void (*scan)(struct virtio_device *dev);
    void (*remove)(struct virtio_device *dev);
    void (*config_changed)(struct virtio_device *dev);
    */
};

int
register_virtio_device(struct virtio_device *dev);

static inline struct virtio_device *dev_to_virtio(struct device *_dev)
{
    return container_of(_dev, struct virtio_device, dev);
}

static inline struct virtio_driver *drv_to_virtio(struct device_driver *drv)
{
    return container_of(drv, struct virtio_driver, driver);
}

#endif /* _LINUX_VIRTIO_H */
