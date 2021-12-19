// SPDX-License-Identifier: GPL-2.0-only

#include <of.h>
#include <irq.h>
#include <export.h>

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

struct device_node *of_get_parent(const struct device_node *node)
{
    if (!node)
        return NULL;

    return of_node_get(node->parent);
}
EXPORT_SYMBOL(of_get_parent);

struct device_node *of_find_node_by_phandle(phandle handle)
{
    struct device_node *np = NULL;

    if (!handle)
        return NULL;

    for_each_of_allnodes(np) {
        if (np->phandle == handle &&
            !of_node_check_flag(np, OF_DETACHED)) {
            break;
        }
    }

    of_node_get(np);
    return np;
}
EXPORT_SYMBOL(of_find_node_by_phandle);

struct device_node *__of_find_all_nodes(struct device_node *prev)
{
    struct device_node *np;
    if (!prev) {
        np = of_root;
    } else if (prev->child) {
        np = prev->child;
    } else {
        /* Walk back up looking for a sibling, or the end of the structure */
        np = prev;
        while (np->parent && !np->sibling)
            np = np->parent;
        np = np->sibling; /* Might be null at the end of the tree */
    }
    return np;
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
                pr_debug("%s: %s, %s\n", __func__, cp, compat);
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
    match = __of_match_node(matches, node);
    return match;
}
EXPORT_SYMBOL(of_match_node);

struct device_node *
of_find_matching_node_and_match(struct device_node *from,
                                const struct of_device_id *matches,
                                const struct of_device_id **match)
{
    unsigned long flags;
    struct device_node *np;
    const struct of_device_id *m;

    if (match)
        *match = NULL;

    for_each_of_allnodes_from(from, np) {
        m = __of_match_node(matches, np);
        if (m && of_node_get(np)) {
            if (match)
                *match = m;
            break;
        }
    }
    of_node_put(from);
    return np;
}
EXPORT_SYMBOL(of_find_matching_node_and_match);

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
    return __of_device_is_available(device);
}
EXPORT_SYMBOL(of_device_is_available);
