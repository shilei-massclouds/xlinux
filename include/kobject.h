// SPDX-License-Identifier: GPL-2.0
#ifndef _KOBJECT_H_
#define _KOBJECT_H_

#include <types.h>
#include <list.h>
#include <kref.h>
#include <uidgid.h>

struct kobject {
    const char          *name;
    list_head           entry;
    struct kobject      *parent;
    struct kset         *kset;
    struct kobj_type    *ktype;
    struct kernfs_node  *sd; /* sysfs directory entry */
    struct kref         kref;
    unsigned int state_initialized:1;
    unsigned int state_in_sysfs:1;
    unsigned int state_add_uevent_sent:1;
    unsigned int state_remove_uevent_sent:1;
    unsigned int uevent_suppress:1;
};

struct kobj_type {
    void (*release)(struct kobject *kobj);
    const struct sysfs_ops *sysfs_ops;
    struct attribute **default_attrs;   /* use default_groups instead */
    const struct attribute_group **default_groups;
    const struct kobj_ns_type_operations *(*child_ns_type)(struct kobject *kobj);
    const void *(*namespace)(struct kobject *kobj);
    void (*get_ownership)(struct kobject *kobj, kuid_t *uid, kgid_t *gid);
};

extern void kobject_init(struct kobject *kobj, struct kobj_type *ktype);

#endif /* _KOBJECT_H_ */
