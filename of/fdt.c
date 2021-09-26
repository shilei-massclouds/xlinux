// SPDX-License-Identifier: GPL-2.0-only
#include <fdt.h>
#include <printk.h>
#include <types.h>
#include <string.h>
#include <bug.h>
#include <errno.h>

void *initial_boot_params;

int dt_root_addr_cells;
int dt_root_size_cells;

static uint32_t
fdt_next_tag(const void *fdt, int startoffset, int *nextoffset)
{
    const fdt32_t *tagp;
    const fdt32_t *lenp;
    uint32_t tag;
    const char *p;
    int offset = startoffset;

    tagp = fdt_offset_ptr_(fdt, offset);
    if (!tagp)
        return FDT_END; /* premature end */

    tag = fdt32_to_cpu(*tagp);
    offset += FDT_TAGSIZE;

    switch (tag) {
    case FDT_BEGIN_NODE:
        /* skip name */
        do {
            p = fdt_offset_ptr_(fdt, offset++);
        } while (p && (*p != '\0'));
        if (!p)
            return FDT_END; /* premature end */
        break;

    case FDT_PROP:
        lenp = fdt_offset_ptr_(fdt, offset);
        if (!lenp)
            return FDT_END; /* premature end */
        /* skip-name offset, length and value */
        offset += sizeof(struct fdt_property)
            - FDT_TAGSIZE + fdt32_to_cpu(*lenp);
        break;

    case FDT_END:
    case FDT_END_NODE:
    case FDT_NOP:
        break;

    default:
        return FDT_END;
    }

    if (!fdt_offset_ptr_(fdt, startoffset))
        return FDT_END; /* premature end */

    *nextoffset = FDT_TAGALIGN(offset);
    return tag;
}

static int
fdt_next_node(const void *fdt, int offset, int *depth)
{
    uint32_t tag;
    int nextoffset = 0;

    if (offset >= 0)
        fdt_next_tag(fdt, offset, &nextoffset);

    do {
        offset = nextoffset;
        tag = fdt_next_tag(fdt, offset, &nextoffset);

        switch (tag) {
        case FDT_PROP:
        case FDT_NOP:
            break;

        case FDT_BEGIN_NODE:
            if (depth)
                (*depth)++;
            break;

        case FDT_END_NODE:
            if (depth && ((--(*depth)) < 0))
                return nextoffset;
            break;

        case FDT_END:
            if ((nextoffset >= 0)
                || ((nextoffset == -FDT_ERR_TRUNCATED) && !depth))
                return -FDT_ERR_NOTFOUND;
            else
                return nextoffset;
        }
    } while (tag != FDT_BEGIN_NODE);

    return offset;
}

static const char *
fdt_get_name(const void *fdt, int nodeoffset, int *len)
{
    const struct fdt_node_header *nh = fdt_offset_ptr_(fdt, nodeoffset);
    const char *nameptr = nh->name;

    if (len)
        *len = strlen(nameptr);

    return nameptr;
}

static int
nextprop_(const void *fdt, int offset)
{
    uint32_t tag;
    int nextoffset;

    do {
        tag = fdt_next_tag(fdt, offset, &nextoffset);

        switch (tag) {
        case FDT_END:
            if (nextoffset >= 0)
                return -FDT_ERR_BADSTRUCTURE;
            else
                return nextoffset;

        case FDT_PROP:
            return offset;
        }
        offset = nextoffset;
    } while (tag == FDT_NOP);

    return -FDT_ERR_NOTFOUND;
}

static int
fdt_first_property_offset(const void *fdt, int nodeoffset)
{
    int offset;
    fdt_next_tag(fdt, nodeoffset, &offset);
    return nextprop_(fdt, offset);
}

static int
fdt_next_property_offset(const void *fdt, int offset)
{
    fdt_next_tag(fdt, offset, &offset);
    return nextprop_(fdt, offset);
}

static const struct fdt_property *
fdt_get_property_by_offset_(const void *fdt, int offset, int *lenp)
{
    const struct fdt_property *prop;

    prop = fdt_offset_ptr_(fdt, offset);

    if (lenp)
        *lenp = fdt32_ld(&prop->len);

    return prop;
}

static const char *
fdt_get_string(const void *fdt, int stroffset, int *lenp)
{
    uint32_t absoffset;
    size_t len;
    const char *s, *n;

    absoffset = stroffset + fdt_off_dt_strings(fdt);
    len = fdt_size_dt_strings(fdt) - stroffset;
    s = (const char *)fdt + absoffset;
    n = memchr(s, '\0', len);

    if (lenp)
        *lenp = n - s;
    return s;
}

static int
fdt_string_eq_(const void *fdt, int stroffset, const char *s, int len)
{
    int slen;
    const char *p = fdt_get_string(fdt, stroffset, &slen);

    return p && (slen == len) && (memcmp(p, s, len) == 0);
}

static const struct fdt_property *
fdt_get_property_namelen_(const void *fdt,
                          int offset,
                          const char *name,
                          int namelen,
                          int *lenp,
                          int *poffset)
{
    for (offset = fdt_first_property_offset(fdt, offset);
         (offset >= 0);
         (offset = fdt_next_property_offset(fdt, offset))) {
        const struct fdt_property *prop;

        prop = fdt_get_property_by_offset_(fdt, offset, lenp);
        if (!prop) {
            offset = -FDT_ERR_INTERNAL;
            break;
        }

        if (fdt_string_eq_(fdt, fdt32_ld(&prop->nameoff), name, namelen)) {
            if (poffset)
                *poffset = offset;
            return prop;
        }
    }

    if (lenp)
        *lenp = offset;
    return NULL;
}

static const void *
fdt_getprop_namelen(const void *fdt,
                    int nodeoffset,
                    const char *name,
                    int namelen,
                    int *lenp)
{
    int poffset;
    const struct fdt_property *prop;

    prop = fdt_get_property_namelen_(fdt, nodeoffset, name, namelen, lenp,
                     &poffset);
    if (!prop)
        return NULL;

    return prop->data;
}

static const void *
fdt_getprop(const void *fdt, int nodeoffset, const char *name, int *lenp)
{
    return fdt_getprop_namelen(fdt, nodeoffset, name, strlen(name), lenp);
}

int
fdt_check_header(const void *fdt)
{
    if (fdt_magic(fdt) != FDT_MAGIC)
        return -FDT_ERR_BADMAGIC;

    return 0;
}

/**
 * of_get_flat_dt_prop - Given a node in the flat blob, return the property ptr
 *
 * This function can be used within scan_flattened_dt callback to get
 * access to properties
 */
const void *
of_get_flat_dt_prop(unsigned long node, const char *name, int *size)
{
    return fdt_getprop(initial_boot_params, node, name, size);
}

/**
 * of_scan_flat_dt - scan flattened tree blob and call callback on each.
 * @it: callback function
 * @data: context data pointer
 *
 * This function is used to scan the flattened device-tree, it is
 * used to extract the memory information at boot before we can
 * unflatten the tree
 */
int
of_scan_flat_dt(of_scan_flat_dt_cb cb, void *data)
{
    const void *blob = initial_boot_params;
    const char *pathp;
    int offset, rc = 0, depth = -1;

    if (!blob)
        return 0;

    for (offset = fdt_next_node(blob, -1, &depth);
         offset >= 0 && depth >= 0 && !rc;
         offset = fdt_next_node(blob, offset, &depth)) {

        pathp = fdt_get_name(blob, offset, NULL);
        rc = cb(offset, pathp, depth, data);
    }

    return rc;
}

static void *
unflatten_dt_alloc(void **mem, unsigned long size, unsigned long align)
{
	void *res;

	*mem = PTR_ALIGN(*mem, align);
	res = *mem;
	*mem += size;

	return res;
}

const void *
fdt_getprop_by_offset(const void *fdt, int offset,
                      const char **namep, int *lenp)
{
    const struct fdt_property *prop;

    prop = fdt_get_property_by_offset_(fdt, offset, lenp);
    if (!prop)
        return NULL;

    if (namep) {
        const char *name;
        int namelen;

        name = fdt_get_string(fdt, fdt32_ld(&prop->nameoff), &namelen);
        if (!name) {
            if (lenp)
                *lenp = namelen;
            return NULL;
        }

        *namep = name;
    }

    return prop->data;
}

static void
populate_properties(const void *blob,
                    int offset,
                    void **mem,
                    struct device_node *np,
                    const char *nodename,
                    bool dryrun)
{
	int cur;
    struct property *pp;
	struct property **pprev = NULL;

	pprev = &np->properties;
	for (cur = fdt_first_property_offset(blob, offset);
	     cur >= 0;
	     cur = fdt_next_property_offset(blob, cur)) {
		const u32 *val;
		const char *pname;
		u32 sz;

		val = fdt_getprop_by_offset(blob, cur, &pname, &sz);
		if (!val) {
			panic("Cannot locate property at %x\n", cur);
			continue;
		}

		if (!pname) {
			panic("Cannot find property name at %x\n", cur);
			continue;
		}

		pp = unflatten_dt_alloc(mem, sizeof(struct property),
                                __alignof__(struct property));
		if (dryrun)
			continue;

		if (!strcmp(pname, "phandle") ||
		    !strcmp(pname, "linux,phandle")) {
			if (!np->phandle)
				np->phandle = be32_to_cpup(val);
		}

		pp->name   = (char *)pname;
		pp->length = sz;
		pp->value  = (u32 *)val;
		*pprev     = pp;
		pprev      = &pp->next;
    }

	{
		const char *p = nodename, *ps = p, *pa = NULL;
		int len;

		while (*p) {
			if ((*p) == '@')
				pa = p;
			else if ((*p) == '/')
				ps = p + 1;
			p++;
		}

		if (pa < ps)
			pa = p;
		len = (pa - ps) + 1;
		pp = unflatten_dt_alloc(mem, sizeof(struct property) + len,
                                __alignof__(struct property));
		if (!dryrun) {
			pp->name   = "name";
			pp->length = len;
			pp->value  = pp + 1;
			*pprev     = pp;
			pprev      = &pp->next;
			memcpy(pp->value, ps, len - 1);
			((char *)pp->value)[len - 1] = 0;
		}
	}

	if (!dryrun)
		*pprev = NULL;
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

static bool
populate_node(const void *blob,
              int offset,
              void **mem,
              struct device_node *dad,
              struct device_node **pnp,
              bool dryrun)
{
	struct device_node *np;
	const char *pathp;
    unsigned int l;
    unsigned int allocl;

	pathp = fdt_get_name(blob, offset, &l);
	if (!pathp) {
		*pnp = NULL;
		return false;
	}

	allocl = ++l;

	np = unflatten_dt_alloc(mem, sizeof(struct device_node) + allocl,
                            __alignof__(struct device_node));
	if (!dryrun) {
		char *fn;
		of_node_init(np);
		np->full_name = fn = ((char *)np) + sizeof(*np);

		memcpy(fn, pathp, l);

		if (dad != NULL) {
			np->parent = dad;
			np->sibling = dad->child;
			dad->child = np;
		}
	}

	populate_properties(blob, offset, mem, np, pathp, dryrun);
	if (!dryrun) {
		np->name = of_get_property(np, "name", NULL);
		if (!np->name)
			np->name = "<NULL>";
	}

	*pnp = np;
    return true;
}

static void
reverse_nodes(struct device_node *parent)
{
	struct device_node *child, *next;

	/* In-depth first */
	child = parent->child;
	while (child) {
		reverse_nodes(child);

		child = child->sibling;
	}

	/* Reverse the nodes in the child list */
	child = parent->child;
	parent->child = NULL;
	while (child) {
		next = child->sibling;

		child->sibling = parent->child;
		parent->child = child;
		child = next;
	}
}

static int
unflatten_dt_nodes(const void *blob,
                   void *mem,
                   struct device_node *dad,
                   struct device_node **nodepp)
{
	struct device_node *root;
	int offset = 0, depth = 0, initial_depth = 0;
#define FDT_MAX_DEPTH	64
	struct device_node *nps[FDT_MAX_DEPTH];
	void *base = mem;
	bool dryrun = !base;

	if (nodepp)
		*nodepp = NULL;

	if (dad)
		depth = initial_depth = 1;

	root = dad;
	nps[depth] = dad;

	for (offset = 0;
	     offset >= 0 && depth >= initial_depth;
	     offset = fdt_next_node(blob, offset, &depth)) {
		if (!populate_node(blob, offset, &mem, nps[depth],
                           &nps[depth+1], dryrun))
			return mem - base;

		if (!dryrun && nodepp && !*nodepp)
            *nodepp = nps[depth+1];
		if (!dryrun && !root)
			root = nps[depth+1];
    }

	if (offset < 0 && offset != -FDT_ERR_NOTFOUND) {
		panic("Error %d processing FDT\n", offset);
		return -EINVAL;
	}

	/*
	 * Reverse the child list. Some drivers assumes node order
     * matches .dts node order
	 */
	if (!dryrun)
		reverse_nodes(root);

	return mem - base;
}

void *
__unflatten_device_tree(const void *blob,
                        struct device_node *dad,
                        struct device_node **mynodes,
                        void *(*dt_alloc)(u64 size, u64 align))
{
	int size;
	void *mem;

    printk(" -> unflatten_device_tree() blob(%lx)\n", blob);

    if (!blob) {
        printk("No device tree pointer\n");
        return NULL;
    }

    if (fdt_check_header(blob)) {
        printk("Invalid device tree blob header\n");
        return NULL;
    }

    printk("Unflattening device tree:\n");
    printk("magic: %x\n", fdt_magic(blob));
    printk("size: %x\n", fdt_totalsize(blob));
    printk("version: %x\n", fdt_version(blob));

	/* First pass, scan for size */
	size = unflatten_dt_nodes(blob, NULL, dad, NULL);
	if (size < 0)
		return NULL;

	size = _ALIGN(size, 4);
	printk("  size is %d, allocating...\n", size);

	/* Allocate memory for the expanded device tree */
	mem = dt_alloc(size + 4, __alignof__(struct device_node));
	if (!mem)
		return NULL;

	memset(mem, 0, size);

	*(u32 *)(mem + size) = cpu_to_be32(0xdeadbeef);

	printk("  unflattening %lx...\n", mem);

	/* Second pass, do actual unflattening */
	unflatten_dt_nodes(blob, mem, dad, mynodes);
	if (be32_to_cpup(mem + size) != 0xdeadbeef)
		panic("End of tree marker overwritten: %08x\n",
              be32_to_cpup(mem + size));

	printk(" <- unflatten_device_tree()\n");
    return mem;
}

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
