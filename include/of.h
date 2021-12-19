/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _LINUX_OF_H
#define _LINUX_OF_H

#include <types.h>
#include <device.h>
#include <fwnode.h>
#include <kernel.h>
#include <string.h>
#include <kobject.h>

#define of_compat_cmp(s1, s2, l)    strcasecmp((s1), (s2))
#define of_prop_cmp(s1, s2)         strcmp((s1), (s2))

/*
 * struct device_node flag descriptions
 * (need to be visible even when !CONFIG_OF)
 */
#define OF_DYNAMIC          1 /* (and properties) allocated via kmalloc */
#define OF_DETACHED         2 /* detached from the device tree */
#define OF_POPULATED        3 /* device already created */
#define OF_POPULATED_BUS    4 /* platform bus created for children */
#define OF_OVERLAY          5 /* allocated for an overlay */
#define OF_OVERLAY_FREE_CSET    6 /* in overlay cset being freed */

#define OF_BAD_ADDR ((u64)-1)
#define for_each_of_allnodes_from(from, dn) \
    for (dn = __of_find_all_nodes(from); dn; dn = __of_find_all_nodes(dn))

#define for_each_of_allnodes(dn) for_each_of_allnodes_from(NULL, dn)

typedef u32 phandle;

struct property {
    char    *name;
    int     length;
    void    *value;

    struct property *next;
};

struct device_node {
	const char *name;
    phandle     phandle;
    const char *full_name;

    struct property *properties;

    struct device_node *parent;
    struct device_node *child;
    struct device_node *sibling;

    struct kobject kobj;
    struct fwnode_handle fwnode;

    unsigned long _flags;
};

#define MAX_PHANDLE_ARGS 16
struct of_phandle_args {
    struct device_node *np;
    int args_count;
    uint32_t args[MAX_PHANDLE_ARGS];
};

extern struct device_node *of_root;

struct device_node *of_get_parent(const struct device_node *node);

static inline struct device_node *
of_node_get(struct device_node *node)
{
    return node;
}

struct device_node *of_find_node_by_phandle(phandle handle);

struct device_node *__of_find_all_nodes(struct device_node *prev);

static inline int
of_node_check_flag(struct device_node *n, unsigned long flag)
{
    return test_bit(flag, &n->_flags);
}

int
of_property_read_u32_index(const struct device_node *np,
                           const char *propname,
                           u32 index, u32 *out_value);

struct property *
__of_find_property(const struct device_node *np, const char *name, int *lenp);

struct property *
of_find_property(const struct device_node *np, const char *name, int *lenp);

static inline void
of_node_put(struct device_node *node)
{
}

const char *
of_prop_next_string(struct property *prop, const char *cur);

const void *
__of_get_property(const struct device_node *np,
                  const char *name, int *lenp);

const struct of_device_id *
of_match_node(const struct of_device_id *matches,
              const struct device_node *node);

bool of_device_is_available(const struct device_node *device);

static inline bool
of_property_read_bool(const struct device_node *np, const char *propname)
{
    struct property *prop = of_find_property(np, propname, NULL);

    return prop ? true : false;
}

extern const struct fwnode_operations of_fwnode_ops;

static inline void
of_node_init(struct device_node *node)
{
    //kobject_init(&node->kobj, &of_node_ktype);
    node->fwnode.ops = &of_fwnode_ops;
}

static inline bool is_of_node(const struct fwnode_handle *fwnode)
{
    return !IS_ERR_OR_NULL(fwnode) && fwnode->ops == &of_fwnode_ops;
}

const struct of_device_id *
of_match_device(const struct of_device_id *matches,
                const struct device *dev);

struct device_node *
of_get_parent(const struct device_node *node);

#endif /* _LINUX_OF_H */
