// SPDX-License-Identifier: GPL-2.0

#ifndef _DEVICE_CLASS_H_
#define _DEVICE_CLASS_H_

#include <device.h>

struct class {
    const char *name;
    struct subsys_private *p;
};

int __class_register(struct class *class);

#define class_register(class)           \
({                                      \
    __class_register(class);            \
})

#endif /* _DEVICE_CLASS_H_ */
