/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TTY_H
#define _LINUX_TTY_H

#include <sysfs.h>
#include <kernel.h>

/* tty driver types */
#define TTY_DRIVER_TYPE_PTY     0x0004

#define TTY_DRIVER_DYNAMIC_ALLOC    0x0040
#define TTY_DRIVER_UNNUMBERED_NODE  0x0080

struct tty_port {
    unsigned char   console:1,      /* port is a console */
                    low_latency:1;  /* optional: tune for latency */
};

struct tty_driver {
    struct cdev **cdevs;
    const char *driver_name;
    const char *name;
    int name_base;      /* offset of printed name */
    int major;          /* major device number */
    int minor_start;    /* start of minor device number */
    unsigned int num;   /* number of devices allocated */
    short type;         /* type of tty driver */
    struct tty_port **ports;
    unsigned long flags;    /* tty driver flags */
};

struct device *
tty_register_device_attr(struct tty_driver *driver,
                         unsigned index,
                         struct device *device,
                         void *drvdata,
                         const struct attribute_group **attr_grp);

struct device *
tty_port_register_device_attr_serdev(struct tty_port *port,
                                     struct tty_driver *driver,
                                     unsigned index,
                                     struct device *device,
                                     void *drvdata,
                                     const struct attribute_group **attr_grp);

extern struct tty_driver *
__tty_alloc_driver(unsigned int lines, unsigned long flags);

/* Use TTY_DRIVER_* flags below */
#define tty_alloc_driver(lines, flags) __tty_alloc_driver(lines, flags)

/*
 * DEPRECATED Do not use this in new code, use tty_alloc_driver instead.
 * (And change the return value checks.)
 */
static inline struct tty_driver *
alloc_tty_driver(unsigned int lines)
{
    struct tty_driver *ret = tty_alloc_driver(lines, 0);
    if (IS_ERR(ret))
        return NULL;
    return ret;
}

void
tty_port_link_device(struct tty_port *port,
                     struct tty_driver *driver, unsigned index);

#endif /* _LINUX_TTY_H */
