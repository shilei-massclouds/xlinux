// SPDX-License-Identifier: GPL-2.0-only
#include <export.h>
#include <sbi.h>
#include <kernel.h>
#include <fdt.h>
#include <page.h>
#include <string.h>
#include <memblock.h>
#include <bug.h>
#include <mm.h>
#include <fwnode.h>

extern void *dtb_early_va;

struct device_node *of_aliases;
struct device_node *of_chosen;
struct device_node *of_stdout;
static const char *of_stdout_options;

struct device_node *of_root;
EXPORT_SYMBOL(of_root);

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
    unsigned long flags;

    next = __of_get_next_child(node, prev);
    return next;
}

#define __for_each_child_of_node(parent, child) \
    for (child = __of_get_next_child(parent, NULL); child != NULL; \
         child = __of_get_next_child(parent, child))

bool
early_init_dt_verify(void)
{
    if (dtb_early_va == NULL)
        panic("dtb_early_va is NULL!");

    if (fdt_check_header(dtb_early_va))
        return false;

    /* Setup flat device-tree pointer */
    initial_boot_params = dtb_early_va;
    return true;
}
EXPORT_SYMBOL(early_init_dt_verify);

/**
 * early_init_dt_scan_root - fetch the top level address and size cells
 */
static int
early_init_dt_scan_root(unsigned long node,
                        const char *uname,
                        int depth,
                        void *data)
{
    const u32 *prop;

    if (depth != 0)
        return 0;

    dt_root_size_cells = OF_ROOT_NODE_SIZE_CELLS_DEFAULT;
    dt_root_addr_cells = OF_ROOT_NODE_ADDR_CELLS_DEFAULT;

    prop = of_get_flat_dt_prop(node, "#size-cells", NULL);
    if (prop)
        dt_root_size_cells = be32_to_cpup(prop);

    prop = of_get_flat_dt_prop(node, "#address-cells", NULL);
    if (prop)
        dt_root_addr_cells = be32_to_cpup(prop);

    /* break now */
    return 1;
}

#define MIN_MEMBLOCK_ADDR   __pa(PAGE_OFFSET)
#define MAX_MEMBLOCK_ADDR   ((phys_addr_t)~0)

static void
early_init_dt_add_memory_arch(u64 base, u64 size)
{
    const u64 phys_offset = MIN_MEMBLOCK_ADDR;

    if (size < PAGE_SIZE - (base & ~PAGE_MASK)) {
        printk("Ignoring memory block %lx - %lx\n",
               base, base + size);
        return;
    }

    if (!PAGE_ALIGNED(base)) {
        size -= PAGE_SIZE - (base & ~PAGE_MASK);
        base = PAGE_ALIGN(base);
    }
    size &= PAGE_MASK;

    if (base > MAX_MEMBLOCK_ADDR) {
        printk("Ignoring memory block %lx - %lx\n",
               base, base + size);
        return;
    }

    if (base + size - 1 > MAX_MEMBLOCK_ADDR) {
        printk("Ignoring memory range %lx - %lx\n",
               ((u64)MAX_MEMBLOCK_ADDR) + 1, base + size);
        size = MAX_MEMBLOCK_ADDR - base + 1;
    }

    if (base + size < phys_offset) {
        printk("Ignoring memory block %lx - %lx\n",
               base, base + size);
        return;
    }

    if (base < phys_offset) {
        printk("Ignoring memory range %lx - %lx\n",
               base, phys_offset);
        size -= phys_offset - base;
        base = phys_offset;
    }

    memblock_add(base, size);
}

static u64
dt_mem_next_cell(int s, const u32 **cellp)
{
    const u32 *p = *cellp;

    *cellp = p + s;
    return of_read_number(p, s);
}

/**
 * early_init_dt_scan_memory - Look for and parse memory nodes
 */
static int
early_init_dt_scan_memory(unsigned long node,
                          const char *uname,
                          int depth,
                          void *data)
{
    const u32 *reg;
    const u32 *endp;
    const char *type;
    int len;

    /* We are scanning "memory" nodes only */
    type = of_get_flat_dt_prop(node, "device_type", NULL);
    if (type == NULL || strcmp(type, "memory") != 0)
        return 0;

    reg = of_get_flat_dt_prop(node, "reg", &len);
    if (reg == NULL)
        return 0;

    endp = reg + (len / sizeof(u32));

    printk("memory scan node %s, reg size %d,\n", uname, len);

    while ((endp - reg) >= (dt_root_addr_cells + dt_root_size_cells)) {
        u64 base, size;

        base = dt_mem_next_cell(dt_root_addr_cells, &reg);
        size = dt_mem_next_cell(dt_root_size_cells, &reg);

        if (size == 0)
            continue;

        printk(" - %lx ,  %lx\n",
               (unsigned long long)base,
               (unsigned long long)size);

        early_init_dt_add_memory_arch(base, size);
    }

    return 0;
}

void
early_init_dt_scan_nodes(void)
{
    of_scan_flat_dt(early_init_dt_scan_root, NULL);

    of_scan_flat_dt(early_init_dt_scan_memory, NULL);
}
EXPORT_SYMBOL(early_init_dt_scan_nodes);

static void *
early_init_dt_alloc_memory_arch(u64 size, u64 align)
{
    void *ptr = memblock_alloc(size, align);

    if (!ptr)
        panic("%s: Failed to allocate %lu bytes align=%lx\n",
              __func__, size, align);

    return ptr;
}

struct device_node *
__of_find_node_by_path(struct device_node *parent, const char *path)
{
	struct device_node *child;
	int len;

	len = strcspn(path, "/:");
	if (!len)
		return NULL;

	__for_each_child_of_node(parent, child) {
		const char *name = kbasename(child->full_name);
		if (strncmp(path, name, len) == 0 && (strlen(name) == len))
			return child;
	}
	return NULL;
}

struct device_node *
__of_find_node_by_full_path(struct device_node *node, const char *path)
{
	const char *separator = strchr(path, ':');

	while (node && *path == '/') {
		struct device_node *tmp = node;

		path++; /* Increment past '/' delimiter */
		node = __of_find_node_by_path(node, path);
		of_node_put(tmp);
		path = strchrnul(path, '/');
		if (separator && separator < path)
			break;
	}
	return node;
}

struct device_node *
of_find_node_opts_by_path(const char *path, const char **opts)
{
	struct device_node *np = NULL;
	const char *separator = strchr(path, ':');

	if (opts)
		*opts = separator ? separator + 1 : NULL;

    if (strcmp(path, "/") == 0)
        return of_node_get(of_root);

	/* The path could begin with an alias */
	if (*path != '/') {
        panic("must begin with /\n");
    }

	if (!np)
		np = of_node_get(of_root);

	np = __of_find_node_by_full_path(np, path);
    return np;
}

static void
of_alias_scan(void * (*dt_alloc)(u64 size, u64 align))
{
	of_aliases = of_find_node_by_path("/aliases");
	if (of_aliases)
        panic("no aliases!");

	of_chosen = of_find_node_by_path("/chosen");
	if (of_chosen == NULL)
		of_chosen = of_find_node_by_path("/chosen@0");

	if (of_chosen) {
        const char *name = NULL;
        if (of_property_read_string(of_chosen, "stdout-path", &name))
            of_property_read_string(of_chosen, "linux,stdout-path", &name);
		if (name)
			of_stdout = of_find_node_opts_by_path(name, &of_stdout_options);
    }
}

void
unflatten_device_tree(void)
{
    __unflatten_device_tree(initial_boot_params, NULL, &of_root,
                            memblock_alloc);

    /* Get pointer to "/chosen" nodes for use everywhere */
    of_alias_scan(memblock_alloc);
    printk("stdout (%s)\n", of_stdout->full_name);
}
EXPORT_SYMBOL(unflatten_device_tree);

static struct fwnode_handle *
of_fwnode_get(struct fwnode_handle *fwnode)
{
    return of_fwnode_handle(of_node_get(to_of_node(fwnode)));
}

static void
of_fwnode_put(struct fwnode_handle *fwnode)
{
    of_node_put(to_of_node(fwnode));
}

const struct fwnode_operations of_fwnode_ops = {
    .get = of_fwnode_get,
    .put = of_fwnode_put,
    /*
    .device_is_available = of_fwnode_device_is_available,
    .device_get_match_data = of_fwnode_device_get_match_data,
    .property_present = of_fwnode_property_present,
    .property_read_int_array = of_fwnode_property_read_int_array,
    .property_read_string_array = of_fwnode_property_read_string_array,
    .get_name = of_fwnode_get_name,
    .get_name_prefix = of_fwnode_get_name_prefix,
    .get_parent = of_fwnode_get_parent,
    .get_next_child_node = of_fwnode_get_next_child_node,
    .get_named_child_node = of_fwnode_get_named_child_node,
    .get_reference_args = of_fwnode_get_reference_args,
    .graph_get_next_endpoint = of_fwnode_graph_get_next_endpoint,
    .graph_get_remote_endpoint = of_fwnode_graph_get_remote_endpoint,
    .graph_get_port_parent = of_fwnode_graph_get_port_parent,
    .graph_parse_endpoint = of_fwnode_graph_parse_endpoint,
    .add_links = of_fwnode_add_links,
    */
};
EXPORT_SYMBOL(of_fwnode_ops);

static void
of_node_release(struct kobject *kobj)
{
    /* Without CONFIG_OF_DYNAMIC, no nodes gets freed */
}

struct kobj_type of_node_ktype = {
    .release = of_node_release,
};

static int
init_module(void)
{
    printk("module[of]: init begin ...\n");
    printk("module[of]: init end!\n");

    return 0;
}
