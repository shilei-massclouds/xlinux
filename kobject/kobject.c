// SPDX-License-Identifier: GPL-2.0
#include <mm.h>
#include <bug.h>
#include <gfp.h>
#include <acgcc.h>
#include <errno.h>
#include <export.h>
#include <printk.h>
#include <string.h>
#include <kobject.h>

static void
kobject_init_internal(struct kobject *kobj)
{
    if (!kobj)
        return;

    kref_init(&kobj->kref);
    INIT_LIST_HEAD(&kobj->entry);
    kobj->state_in_sysfs = 0;
    kobj->state_add_uevent_sent = 0;
    kobj->state_remove_uevent_sent = 0;
    kobj->state_initialized = 1;
}

void
kobject_init(struct kobject *kobj, struct kobj_type *ktype)
{
    char *err_str;

    if (!kobj) {
        err_str = "invalid kobject pointer!";
        goto error;
    }
    if (!ktype) {
        err_str = "must have a ktype to be initialized properly!\n";
        goto error;
    }
    if (kobj->state_initialized) {
        /* do not error out as sometimes we can recover */
        panic("kobject: tried to init an initialized object.\n");
    }

    kobject_init_internal(kobj);
    kobj->ktype = ktype;
    return;

error:
    panic("kobject (%p): %s\n", kobj, err_str);
}
EXPORT_SYMBOL(kobject_init);

static void
kobject_release(struct kref *kref)
{
}

void
kobject_put(struct kobject *kobj)
{
    /* Todo: implement it */
}
EXPORT_SYMBOL(kobject_put);

int
kobject_set_name_vargs(struct kobject *kobj,
                       const char *fmt,
                       va_list vargs)
{
    const char *s;

    if (kobj->name && !fmt)
        return 0;

    s = kvasprintf_const(GFP_KERNEL, fmt, vargs);
    if (!s)
        return -ENOMEM;

    /*
     * ewww... some of these buggers have '/' in the name ... If
     * that's the case, we need to make sure we have an actual
     * allocated copy to modify, since kvasprintf_const may have
     * returned something from .rodata.
     */
    if (strchr(s, '/')) {
        char *t;

        t = kstrdup(s, GFP_KERNEL);
        kfree_const(s);
        if (!t)
            return -ENOMEM;
        strreplace(t, '/', '!');
        s = t;
    }
    kfree_const(kobj->name);
    kobj->name = s;

    return 0;
}

static int
init_module(void)
{
    printk("module[kobject]: init begin ...\n");
    printk("module[kobject]: init end!\n");
    return 0;
}
