// SPDX-License-Identifier: GPL-2.0
#include <kobject.h>
#include <bug.h>
#include <export.h>

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

static int
init_module(void)
{
    sbi_puts("module[kobject]: init begin ...\n");
    sbi_puts("module[kobject]: init end!\n");
    return 0;
}
