/* SPDX-License-Identifier: GPL-2.0 */

#include <tty.h>
#include <cdev.h>
#include <slab.h>
#include <major.h>
#include <export.h>
#include <kdev_t.h>

static struct cdev console_cdev;

static struct tty_driver *
tty_lookup_driver(dev_t device, struct file *filp, int *index)
{
    struct tty_driver *driver = NULL;

    switch (device) {
    case MKDEV(TTYAUX_MAJOR, 1): {
        struct tty_driver *console_driver = console_device(index);
        printk("%s: 2\n", __func__);
        if (console_driver) {
            printk("%s: 3\n", __func__);
            driver = console_driver;
            if (driver && filp) {
                /* Don't let /dev/console block */
                filp->f_flags |= O_NONBLOCK;
                break;
            }
        }
        return ERR_PTR(-ENODEV);
    }
    default:
        panic("no driver!");
        break;
    }
    printk("%s: ! %lx\n", __func__, driver);
    return driver;
}

static struct tty_struct *
tty_open_by_driver(dev_t device, struct file *filp)
{
    int index = -1;
    struct tty_driver *driver = NULL;

    printk("%s: 0\n", __func__);
    driver = tty_lookup_driver(device, filp, &index);

    if (driver)
        printk("%s: 1 name(%s,%s) mm(%x,%x)\n",
               __func__, driver->name, driver->driver_name,
               driver->major, driver->minor_start);

    panic("%s: device(%lx)!", __func__, device);
}

static int tty_open(struct inode *inode, struct file *filp)
{
    struct tty_struct *tty;
    dev_t device = inode->i_rdev;
    printk("%s: dev(%x)\n", __func__, device);

    tty = tty_open_by_driver(device, filp);
    panic("%s: device(%lx)!", __func__, device);
}

static const struct file_operations console_fops = {
    .open       = tty_open,
    /*
    .llseek     = no_llseek,
    .read       = tty_read,
    .write      = redirected_tty_write,
    .poll       = tty_poll,
    .unlocked_ioctl = tty_ioctl,
    .compat_ioctl   = tty_compat_ioctl,
    .release    = tty_release,
    .fasync     = tty_fasync,
    */
};

static int
tty_cdev_add(struct tty_driver *driver, dev_t dev,
             unsigned int index, unsigned int count)
{
    int err;

    /* init here, since reused cdevs cause crashes */
    driver->cdevs[index] = cdev_alloc();
    if (!driver->cdevs[index])
        return -ENOMEM;
    //driver->cdevs[index]->ops = &tty_fops;
    err = cdev_add(driver->cdevs[index], dev, count);
    if (err)
        kobject_put(&driver->cdevs[index]->kobj);
    return err;
}

/**
 *  tty_line_name   -   generate name for a tty
 *  @driver: the tty driver in use
 *  @index: the minor number
 *  @p: output buffer of at least 7 bytes
 *
 *  Generate a name from a driver reference and write it to the output
 *  buffer.
 *
 *  Locking: None
 */
static ssize_t
tty_line_name(struct tty_driver *driver, int index, char *p)
{
    if (driver->flags & TTY_DRIVER_UNNUMBERED_NODE)
        return sprintf(p, "%s", driver->name);
    else
        return sprintf(p, "%s%d", driver->name,
                       index + driver->name_base);
}

struct device *
tty_register_device_attr(struct tty_driver *driver,
                         unsigned index,
                         struct device *device,
                         void *drvdata,
                         const struct attribute_group **attr_grp)
{
    int retval;
    char name[64];
    struct device *dev;
    dev_t devt = MKDEV(driver->major, driver->minor_start) + index;

    if (index >= driver->num)
        panic("%s: Attempt to register invalid tty line number (%d)",
              driver->name, index);

    if (driver->type == TTY_DRIVER_TYPE_PTY)
        panic("TTY_DRIVER_TYPE_PTY!");
    else
        tty_line_name(driver, index, name);

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return ERR_PTR(-ENOMEM);

    dev->devt = devt;
    //dev->class = tty_class;
    dev->parent = device;
    dev_set_name(dev, "%s", name);
    dev_set_drvdata(dev, drvdata);

    printk("%s: devt(%lx) (%lx,%lx)\n",
           __func__, devt, driver->major, driver->minor_start);

    if (!(driver->flags & TTY_DRIVER_DYNAMIC_ALLOC)) {
        retval = tty_cdev_add(driver, devt, index, 1);
        if (retval)
            panic("tty: add cdev error!");
    }
    return dev;
}

void
tty_port_link_device(struct tty_port *port,
                     struct tty_driver *driver, unsigned index)
{
    BUG_ON(index >= driver->num);
    driver->ports[index] = port;
}
EXPORT_SYMBOL(tty_port_link_device);

struct device *
tty_port_register_device_attr_serdev(struct tty_port *port,
                                     struct tty_driver *driver,
                                     unsigned index,
                                     struct device *device,
                                     void *drvdata,
                                     const struct attribute_group **attr_grp)
{
    struct device *dev;

    tty_port_link_device(port, driver, index);

    return tty_register_device_attr(driver, index, device, drvdata,
                                    attr_grp);
}
EXPORT_SYMBOL(tty_port_register_device_attr_serdev);

/**
 * __tty_alloc_driver -- allocate tty driver
 * @lines: count of lines this driver can handle at most
 * @owner: module which is responsible for this driver
 * @flags: some of TTY_DRIVER_* flags, will be set in driver->flags
 *
 * This should not be called directly, some of the provided macros should be
 * used instead. Use IS_ERR and friends on @retval.
 */
struct tty_driver *
__tty_alloc_driver(unsigned int lines, unsigned long flags)
{
    struct tty_driver *driver;
    unsigned int cdevs = 1;

    driver = kzalloc(sizeof(*driver), GFP_KERNEL);
    if (!driver)
        return ERR_PTR(-ENOMEM);

    driver->num = lines;
    driver->flags = flags;

    if (!(flags & TTY_DRIVER_DYNAMIC_ALLOC)) {
        driver->ports = kcalloc(lines, sizeof(*driver->ports),
                                GFP_KERNEL);
        if (!driver->ports)
            panic("Out of memory!");
        cdevs = lines;
    }

    driver->cdevs = kcalloc(cdevs, sizeof(*driver->cdevs), GFP_KERNEL);
    if (!driver->cdevs)
        panic("Out of memory!");

    return driver;
}
EXPORT_SYMBOL(__tty_alloc_driver);

int tty_init(void)
{
    cdev_init(&console_cdev, &console_fops);
    if (cdev_add(&console_cdev, MKDEV(TTYAUX_MAJOR, 1), 1) ||
        register_chrdev_region(MKDEV(TTYAUX_MAJOR, 1), 1, "/dev/console") < 0)
        panic("Couldn't register /dev/console driver\n");

    return 0;
}
