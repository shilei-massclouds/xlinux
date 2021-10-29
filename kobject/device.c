// SPDX-License-Identifier: GPL-2.0
#include <slab.h>
#include <acgcc.h>
#include <errno.h>
#include <printk.h>
#include <export.h>
#include <device.h>

enum devm_ioremap_type {
    DEVM_IOREMAP = 0,
    DEVM_IOREMAP_UC,
    DEVM_IOREMAP_WC,
};

struct devres_node {
    struct list_head        entry;
    dr_release_t            release;
};

struct devres {
    struct devres_node      node;
    /*
     * Some archs want to perform DMA into kmalloc caches
     * and need a guaranteed alignment larger than
     * the alignment of a 64-bit integer.
     * Thus we use ARCH_KMALLOC_MINALIGN here and get exactly the same
     * buffer alignment as if it was allocated by plain kmalloc().
     */
    u8 __aligned(ARCH_KMALLOC_MINALIGN) data[];
};

static int
device_private_init(struct device *dev)
{
    dev->p = kzalloc(sizeof(*dev->p), GFP_KERNEL);
    if (!dev->p)
        return -ENOMEM;
    dev->p->device = dev;
    return 0;
}

int device_add(struct device *dev)
{
    int error = -EINVAL;

    pr_debug("device: '%s': %s\n", dev_name(dev), __func__);

    if (!dev->p) {
        error = device_private_init(dev);
        if (error)
            return error;
    }

    error = bus_add_device(dev);
    if (error)
        return error;

    return 0;
}
EXPORT_SYMBOL(device_add);

/**
 * put_device - decrement reference count.
 * @dev: device in question.
 */
void put_device(struct device *dev)
{
    /* Todo: add kobject_put */
}
EXPORT_SYMBOL(put_device);

/**
 * dev_set_name - set a device name
 * @dev: device
 * @fmt: format string for the device's name
 */
int
dev_set_name(struct device *dev, const char *fmt, ...)
{
    va_list vargs;
    int err;

    va_start(vargs, fmt);
    err = kobject_set_name_vargs(&dev->kobj, fmt, vargs);
    va_end(vargs);
    return err;
}
EXPORT_SYMBOL(dev_set_name);

static bool
check_dr_size(size_t size, size_t *tot_size)
{
    *tot_size = sizeof(struct devres) + size;
    return true;
}

static __always_inline struct devres *
alloc_dr(dr_release_t release, size_t size, gfp_t gfp)
{
    struct devres *dr;
    size_t tot_size;

    if (!check_dr_size(size, &tot_size))
        return NULL;

    dr = kmalloc(tot_size, gfp);
    if (unlikely(!dr))
        return NULL;

    memset(dr, 0, offsetof(struct devres, data));

    INIT_LIST_HEAD(&dr->node.entry);
    dr->node.release = release;
    return dr;
}

static void
devm_kmalloc_release(struct device *dev, void *res)
{
    /* noop */
}

static void
add_dr(struct device *dev, struct devres_node *node)
{
    BUG_ON(!list_empty(&node->entry));
    list_add_tail(&node->entry, &dev->devres_head);
}

void
devres_add(struct device *dev, void *res)
{
    struct devres *dr = container_of(res, struct devres, data);
    unsigned long flags;

    add_dr(dev, &dr->node);
}
EXPORT_SYMBOL(devres_add);

void *
devm_kmalloc(struct device *dev, size_t size, gfp_t gfp)
{
    struct devres *dr;

    if (unlikely(!size))
        return ZERO_SIZE_PTR;

    /* use raw alloc_dr for kmalloc caller tracing */
    dr = alloc_dr(devm_kmalloc_release, size, gfp);
    if (unlikely(!dr))
        return NULL;

    /*
     * This is named devm_kzalloc_release for historical reasons
     * The initial implementation did not support kmalloc, only kzalloc
     */
    devres_add(dev, dr->data);
    return dr->data;
}
EXPORT_SYMBOL(devm_kmalloc);

void
device_initialize(struct device *dev)
{
    kobject_init(&dev->kobj);
    INIT_LIST_HEAD(&dev->devres_head);
}
EXPORT_SYMBOL(device_initialize);

static void *
__devm_ioremap_resource(struct device *dev,
                        const struct resource *res,
                        enum devm_ioremap_type type)
{
    panic("%s: res(%lx) type(%u)!", __func__, res, type);
}

void *
devm_ioremap_resource(struct device *dev, const struct resource *res)
{
    return __devm_ioremap_resource(dev, res, DEVM_IOREMAP);
}
EXPORT_SYMBOL(devm_ioremap_resource);

int
device_register(struct device *dev)
{
    device_initialize(dev);
    return device_add(dev);
}
EXPORT_SYMBOL(device_register);
