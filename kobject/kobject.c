// SPDX-License-Identifier: GPL-2.0
#include <mm.h>
#include <bug.h>
#include <gfp.h>
#include <slab.h>
#include <acgcc.h>
#include <errno.h>
#include <export.h>
#include <printk.h>
#include <string.h>
#include <kobject.h>

/*
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
        panic("kobject: tried to init an initialized object.\n");
    }

    kobject_init_internal(kobj);
    kobj->ktype = ktype;
    return;

error:
    panic("kobject (%p): %s\n", kobj, err_str);
}
EXPORT_SYMBOL(kobject_init);
*/

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
kobject_set_name(struct kobject *kobj, const char *fmt, ...)
{
    va_list vargs;
    int retval;

    va_start(vargs, fmt);
    retval = kobject_set_name_vargs(kobj, fmt, vargs);
    va_end(vargs);

    return retval;
}
EXPORT_SYMBOL(kobject_set_name);

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

struct kobject *
kobject_get_unless_zero(struct kobject *kobj)
{
    if (!kobj)
        return NULL;
    return kobj;
}
EXPORT_SYMBOL(kobject_get_unless_zero);

struct kobject *
kset_find_obj(struct kset *kset, const char *name)
{
    struct kobject *k;
    struct kobject *ret = NULL;

    list_for_each_entry(k, &kset->list, entry) {
        if (kobject_name(k) && !strcmp(kobject_name(k), name)) {
            ret = kobject_get_unless_zero(k);
            break;
        }
    }

    return ret;
}
EXPORT_SYMBOL(kset_find_obj);

static struct kset *
kset_create(const char *name)
{
    struct kset *kset;
    int retval;

    kset = kzalloc(sizeof(*kset), GFP_KERNEL);
    if (!kset)
        return NULL;
    retval = kobject_set_name(&kset->kobj, "%s", name);
    if (retval) {
        kfree(kset);
        return NULL;
    }
    return kset;
}

static void
kobject_init_internal(struct kobject *kobj)
{
    if (!kobj)
        return;
    INIT_LIST_HEAD(&kobj->entry);
}

void
kset_init(struct kset *k)
{
    kobject_init_internal(&k->kobj);
    INIT_LIST_HEAD(&k->list);
}

int
kset_register(struct kset *k)
{
    int err;

    if (!k)
        return -EINVAL;

    kset_init(k);
    return 0;
}
EXPORT_SYMBOL(kset_register);

struct kset *
kset_create_and_add(const char *name)
{
    struct kset *kset;
    int error;

    kset = kset_create(name);
    if (!kset)
        return NULL;
    error = kset_register(kset);
    if (error) {
        kfree(kset);
        return NULL;
    }
    return kset;
}
EXPORT_SYMBOL(kset_create_and_add);

static int
init_module(void)
{
    printk("module[kobject]: init begin ...\n");
    printk("module[kobject]: init end!\n");
    return 0;
}
