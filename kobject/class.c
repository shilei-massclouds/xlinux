// SPDX-License-Identifier: GPL-2.0

#include <slab.h>
#include <class.h>
#include <errno.h>
#include <klist.h>
#include <export.h>

int
__class_register(struct class *cls)
{
    struct subsys_private *cp;

    cp = kzalloc(sizeof(*cp), GFP_KERNEL);
    if (!cp)
        return -ENOMEM;

    klist_init(&cp->klist_devices);
    cp->class = cls;
    cls->p = cp;
    return 0;
}
EXPORT_SYMBOL(__class_register);
