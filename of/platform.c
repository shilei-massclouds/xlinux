// SPDX-License-Identifier: GPL-2.0+
#include <bug.h>
#include <fdt.h>
#include <slab.h>
#include <device.h>
#include <errno.h>
#include <string.h>
#include <export.h>
#include <of_platform.h>

const struct of_device_id
of_default_bus_match_table[] = {
    { .compatible = "simple-bus", },
    { .compatible = "simple-mfd", },
    { .compatible = "isa", },
    {} /* Empty terminated list */
};

struct bus_type
platform_bus_type = {
    .name       = "platform",
    /*
    .dev_groups = platform_dev_groups,
    .match      = platform_match,
    .uevent     = platform_uevent,
    .dma_configure  = platform_dma_configure,
    .pm     = &platform_dev_pm_ops,
    */
};
EXPORT_SYMBOL(platform_bus_type);

static bool
__of_node_is_type(const struct device_node *np, const char *type)
{
    const char *match = __of_get_property(np, "device_type", NULL);

    return np && match && type && !strcmp(match, type);
}

static bool
of_node_name_eq(const struct device_node *np, const char *name)
{
    const char *node_name;
    size_t len;

    if (!np)
        return false;

    node_name = kbasename(np->full_name);
    len = strchrnul(node_name, '@') - node_name;

    return (strlen(name) == len) && (strncmp(node_name, name, len) == 0);
}

struct property *
__of_find_property(const struct device_node *np, const char *name, int *lenp)
{
    struct property *pp;

    if (!np)
        return NULL;

    for (pp = np->properties; pp; pp = pp->next) {
        if (of_prop_cmp(pp->name, name) == 0) {
            if (lenp)
                *lenp = pp->length;
            break;
        }
    }

    return pp;
}

struct property *
of_find_property(const struct device_node *np, const char *name, int *lenp)
{
    struct property *pp;
    unsigned long flags;

    //raw_spin_lock_irqsave(&devtree_lock, flags);
    pp = __of_find_property(np, name, lenp);
    //raw_spin_unlock_irqrestore(&devtree_lock, flags);

    return pp;
}


const void *
__of_get_property(const struct device_node *np,
                  const char *name, int *lenp)
{
    struct property *pp = __of_find_property(np, name, lenp);

    return pp ? pp->value : NULL;
}

const void *
of_get_property(const struct device_node *np, const char *name, int *lenp)
{
    struct property *pp = of_find_property(np, name, lenp);

    return pp ? pp->value : NULL;
}



static int
__of_device_is_compatible(const struct device_node *device,
                          const char *compat,
                          const char *type,
                          const char *name)
{
    struct property *prop;
    const char *cp;
    int index = 0, score = 0;

    /* Compatible match has highest priority */
    if (compat && compat[0]) {
        prop = __of_find_property(device, "compatible", NULL);
        for (cp = of_prop_next_string(prop, NULL); cp;
             cp = of_prop_next_string(prop, cp), index++) {
            if (of_compat_cmp(cp, compat, strlen(compat)) == 0) {
                printk("%s: %s, %s\n", __func__, cp, compat);
                score = INT_MAX/2 - (index << 2);
                break;
            }
        }
        if (!score)
            return 0;
    }

    /* Matching type is better than matching name */
    if (type && type[0]) {
        if (!__of_node_is_type(device, type))
            return 0;
        score += 2;
    }

    /* Matching name is a bit better than not */
    if (name && name[0]) {
        if (!of_node_name_eq(device, name))
            return 0;
        score++;
    }

    return score;
}

static const struct of_device_id *
__of_match_node(const struct of_device_id *matches,
                const struct device_node *node)
{
    const struct of_device_id *best_match = NULL;
    int score, best_score = 0;

    if (!matches)
        return NULL;

    for (;
         matches->name[0] || matches->type[0] || matches->compatible[0];
         matches++) {
        score = __of_device_is_compatible(node, matches->compatible,
                                          matches->type, matches->name);
        if (score > best_score) {
            best_match = matches;
            best_score = score;
        }
    }

    return best_match;
}

const struct of_device_id *
of_match_node(const struct of_device_id *matches,
              const struct device_node *node)
{
    const struct of_device_id *match;
    //unsigned long flags;

    //raw_spin_lock_irqsave(&devtree_lock, flags);
    match = __of_match_node(matches, node);
    //raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return match;
}

static const struct of_dev_auxdata *
of_dev_lookup(const struct of_dev_auxdata *lookup,
              struct device_node *np)
{
    if (!lookup)
        return NULL;

    panic("Cant come here!!!\n");
}

struct platform_object {
    struct platform_device pdev;
    char name[];
};

struct platform_device *
platform_device_alloc(const char *name, int id)
{
    struct platform_object *pa;

    pa = kzalloc(sizeof(*pa) + strlen(name) + 1, GFP_KERNEL);

    return pa ? &pa->pdev : NULL;
}

static void
of_device_make_bus_id(struct device *dev)
{
    u64 addr;
    struct device_node *node = dev->of_node;

    while (node->parent) {
        /* format arguments only used if dev_name() resolves to NULL */
        dev_set_name(dev, dev_name(dev) ? "%s:%s" : "%s",
                     kbasename(node->full_name), dev_name(dev));
        node = node->parent;
    }
}

struct platform_device *
of_device_alloc(struct device_node *np,
                const char *bus_id,
                struct device *parent)
{
    struct platform_device *dev;

    dev = platform_device_alloc("", PLATFORM_DEVID_NONE);
    if (!dev)
        return NULL;

    printk("%s: %s bus_id(%s) parent(%lx)\n",
           __func__, np->full_name, bus_id, parent);

    dev->dev.of_node = of_node_get(np);

    if (bus_id)
        dev_set_name(&dev->dev, "%s", bus_id);
    else
        of_device_make_bus_id(&dev->dev);

    return dev;
}

int
of_device_add(struct platform_device *ofdev)
{
    ofdev->name = dev_name(&ofdev->dev);
    printk("%s: %s\n", __func__, ofdev->name);
    ofdev->id = PLATFORM_DEVID_NONE;
    return device_add(&ofdev->dev);
}

void
platform_device_put(struct platform_device *pdev)
{
    if (!IS_ERR_OR_NULL(pdev))
        put_device(&pdev->dev);
}

static bool
__of_device_is_available(const struct device_node *device)
{
    const char *status;
    int statlen;

    if (!device)
        return false;

    status = __of_get_property(device, "status", &statlen);
    if (status == NULL)
        return true;

    if (statlen > 0) {
        if (!strcmp(status, "okay") || !strcmp(status, "ok"))
            return true;
    }

    return false;
}

bool
of_device_is_available(const struct device_node *device)
{
    bool res;
    res = __of_device_is_available(device);
    return res;
}
EXPORT_SYMBOL(of_device_is_available);

static struct platform_device *
of_platform_device_create_pdata(struct device_node *np,
                                const char *bus_id,
                                void *platform_data,
                                struct device *parent)
{
    struct platform_device *dev;

    if (!of_device_is_available(np) ||
        of_node_test_and_set_flag(np, OF_POPULATED))
        return NULL;

    dev = of_device_alloc(np, bus_id, parent);
    if (!dev)
        goto err_clear_flag;

    dev->dev.bus = &platform_bus_type;
    dev->dev.platform_data = platform_data;

    if (of_device_add(dev) != 0) {
        platform_device_put(dev);
        goto err_clear_flag;
    }

    return dev;

 err_clear_flag:
    of_node_clear_flag(np, OF_POPULATED);
    return NULL;
}

static int
of_platform_bus_create(struct device_node *bus,
                       const struct of_device_id *matches,
                       const struct of_dev_auxdata *lookup,
                       struct device *parent,
                       bool strict)
{
    const char *bus_id = NULL;
    void *platform_data = NULL;
    struct platform_device *dev;
    struct device_node *child = NULL;
    const struct of_dev_auxdata *auxdata;
    int rc = 0;

    printk("%s\n", bus->full_name);

    /* Make sure it has a compatible property */
    if (strict && (!of_get_property(bus, "compatible", NULL))) {
        return 0;
    }

    if (of_node_check_flag(bus, OF_POPULATED_BUS)) {
        printk("%s() - skipping %lxOF, already populated\n",
               __func__, bus);
        return 0;
    }

    auxdata = of_dev_lookup(lookup, bus);
    if (auxdata) {
        bus_id = auxdata->name;
        platform_data = auxdata->platform_data;
    }

    dev = of_platform_device_create_pdata(bus, bus_id, platform_data, parent);
    if (!dev || !of_match_node(matches, bus))
        return 0;

    for_each_child_of_node(bus, child) {
        printk("   create child: %s\n", child->full_name);
        rc = of_platform_bus_create(child, matches, lookup, &dev->dev, strict);
        if (rc) {
            of_node_put(child);
            break;
        }
    }
    of_node_set_flag(bus, OF_POPULATED_BUS);
    return rc;
}

static int
of_platform_populate(struct device_node *root,
                     const struct of_device_id *matches,
                     const struct of_dev_auxdata *lookup,
                     struct device *parent)
{
    struct device_node *child;
    int rc = 0;

    root = root ? of_node_get(root) : of_find_node_by_path("/");
    if (!root)
        return -EINVAL;

    printk("%s()\n", __func__);

    for_each_child_of_node(root, child) {
        rc = of_platform_bus_create(child, matches, lookup, parent, true);
        if (rc) {
            of_node_put(child);
            break;
        }
    }

    of_node_set_flag(root, OF_POPULATED_BUS);

    of_node_put(root);
    return rc;
}

static struct device_node *
__of_get_next_child(const struct device_node *node,
                    struct device_node *prev)
{
    struct device_node *next;

    if (!node)
        return NULL;

    next = prev ? prev->sibling : node->child;
    for (; next; next = next->sibling)
        if (of_node_get(next))
            break;
    of_node_put(prev);
    return next;
}

struct device_node *
of_get_next_child(const struct device_node *node,
                  struct device_node *prev)
{
    struct device_node *next;
    next = __of_get_next_child(node, prev);
    return next;
}
EXPORT_SYMBOL(of_get_next_child);

const char *
of_prop_next_string(struct property *prop, const char *cur)
{
    const void *curv = cur;

    if (!prop)
        return NULL;

    if (!cur)
        return prop->value;

    curv += strlen(cur) + 1;
    if (curv >= prop->value + prop->length)
        return NULL;

    return curv;
}
EXPORT_SYMBOL(of_prop_next_string);

int
of_property_read_string(const struct device_node *np,
                        const char *propname,
                        const char **out_string)
{
    const struct property *prop = of_find_property(np, propname, NULL);
    if (!prop)
        return -EINVAL;
    if (!prop->value)
        return -ENODATA;
    if (strnlen(prop->value, prop->length) >= prop->length)
        return -EILSEQ;
    *out_string = prop->value;
    return 0;
}
EXPORT_SYMBOL(of_property_read_string);


int
of_platform_default_populate_init(void)
{
    if (!of_have_populated_dt())
        return -ENODEV;

    return of_platform_populate(NULL, of_default_bus_match_table,
                                NULL, NULL);
}
EXPORT_SYMBOL(of_platform_default_populate_init);

int
platform_bus_init(void)
{
    int error;
    error = bus_register(&platform_bus_type);
    if (error)
        panic("cant register platform bus type!");

    return error;
}
EXPORT_SYMBOL(platform_bus_init);
