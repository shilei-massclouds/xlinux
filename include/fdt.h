// SPDX-License-Identifier: GPL-2.0-only
#ifndef LIBFDT_H
#define LIBFDT_H

#include <types.h>
#include <fwnode.h>
#include <kobject.h>
#include <kernel.h>

#define of_prop_cmp(s1, s2)     strcmp((s1), (s2))

typedef u32 phandle;

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
};

extern struct kobj_type of_node_ktype;
extern const struct fwnode_operations of_fwnode_ops;

static inline void
of_node_init(struct device_node *node)
{
    kobject_init(&node->kobj, &of_node_ktype);
    node->fwnode.ops = &of_fwnode_ops;
}

#define OF_ROOT_NODE_ADDR_CELLS_DEFAULT 1
#define OF_ROOT_NODE_SIZE_CELLS_DEFAULT 1

#define FDT_MAGIC       0xd00dfeed  /* 4: version, 4: total size */
#define FDT_TAGSIZE     sizeof(fdt32_t)

#define FDT_ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define FDT_TAGALIGN(x) (FDT_ALIGN((x), FDT_TAGSIZE))

/* Tag */
#define FDT_BEGIN_NODE  0x1     /* Start node: full name */
#define FDT_END_NODE    0x2     /* End node */
#define FDT_PROP        0x3     /* Property: name off, size, content */
#define FDT_NOP         0x4     /* nop */
#define FDT_END         0x9

#define fdt32_to_cpu(x) be32_to_cpu(x)

#define fdt_get_header(fdt, field) \
    (fdt32_ld(&((const struct fdt_header *)(fdt))->field))

#define fdt_magic(fdt)              (fdt_get_header(fdt, magic))
#define fdt_totalsize(fdt)          (fdt_get_header(fdt, totalsize))
#define fdt_off_dt_struct(fdt)      (fdt_get_header(fdt, off_dt_struct))
#define fdt_version(fdt)            (fdt_get_header(fdt, version))
#define fdt_off_dt_strings(fdt)     (fdt_get_header(fdt, off_dt_strings))
#define fdt_size_dt_strings(fdt)    (fdt_get_header(fdt, size_dt_strings))

typedef u32 fdt32_t;

struct fdt_node_header {
    fdt32_t tag;
    char name[0];
};

struct fdt_property {
    fdt32_t tag;
    fdt32_t len;
    fdt32_t nameoff;
    char data[0];
};

struct fdt_header {
    fdt32_t magic;              /* magic word FDT_MAGIC */
    fdt32_t totalsize;          /* total size of DT block */
    fdt32_t off_dt_struct;      /* offset to structure */
    fdt32_t off_dt_strings;     /* offset to strings */
    fdt32_t off_mem_rsvmap;     /* offset to memory reserve map */
    fdt32_t version;            /* format version */
    fdt32_t last_comp_version;  /* last compatible version */

    /* version 2 fields below */
    fdt32_t boot_cpuid_phys;    /* Which physical CPU id we're booting on */
    /* version 3 fields below */
    fdt32_t size_dt_strings;    /* size of the strings block */

    /* version 17 fields below */
    fdt32_t size_dt_struct;     /* size of the structure block */
};

extern void *initial_boot_params;
extern int dt_root_addr_cells;
extern int dt_root_size_cells;

int
fdt_check_header(const void *fdt);

const void *
of_get_flat_dt_prop(unsigned long node, const char *name, int *size);

static inline uint32_t
fdt32_ld(const fdt32_t *p)
{
    const uint8_t *bp = (const uint8_t *)p;

    return ((uint32_t)bp[0] << 24) | ((uint32_t)bp[1] << 16) |
        ((uint32_t)bp[2] << 8) | bp[3];
}

/* Error codes: informative error codes */
#define FDT_ERR_NOTFOUND    1
    /* FDT_ERR_NOTFOUND: The requested node or property does not exist */
#define FDT_ERR_EXISTS      2
    /* FDT_ERR_EXISTS: Attempted to create a node or property which
     * already exists */
#define FDT_ERR_NOSPACE     3
    /* FDT_ERR_NOSPACE: Operation needed to expand the device
     * tree, but its buffer did not have sufficient space to
     * contain the expanded tree. Use fdt_open_into() to move the
     * device tree to a buffer with more space. */

/* Error codes: codes for bad parameters */
#define FDT_ERR_BADOFFSET   4
    /* FDT_ERR_BADOFFSET: Function was passed a structure block
     * offset which is out-of-bounds, or which points to an
     * unsuitable part of the structure for the operation. */
#define FDT_ERR_BADPATH     5
    /* FDT_ERR_BADPATH: Function was passed a badly formatted path
     * (e.g. missing a leading / for a function which requires an
     * absolute path) */
#define FDT_ERR_BADPHANDLE  6
    /* FDT_ERR_BADPHANDLE: Function was passed an invalid phandle.
     * This can be caused either by an invalid phandle property
     * length, or the phandle value was either 0 or -1, which are
     * not permitted. */
#define FDT_ERR_BADSTATE    7
    /* FDT_ERR_BADSTATE: Function was passed an incomplete device
     * tree created by the sequential-write functions, which is
     * not sufficiently complete for the requested operation. */

/* Error codes: codes for bad device tree blobs */
#define FDT_ERR_TRUNCATED   8
    /* FDT_ERR_TRUNCATED: FDT or a sub-block is improperly
     * terminated (overflows, goes outside allowed bounds, or
     * isn't properly terminated).  */
#define FDT_ERR_BADMAGIC    9
    /* FDT_ERR_BADMAGIC: Given "device tree" appears not to be a
     * device tree at all - it is missing the flattened device
     * tree magic number. */
#define FDT_ERR_BADVERSION  10
    /* FDT_ERR_BADVERSION: Given device tree has a version which
     * can't be handled by the requested operation.  For
     * read-write functions, this may mean that fdt_open_into() is
     * required to convert the tree to the expected version. */
#define FDT_ERR_BADSTRUCTURE    11
    /* FDT_ERR_BADSTRUCTURE: Given device tree has a corrupt
     * structure block or other serious error (e.g. misnested
     * nodes, or subnodes preceding properties). */
#define FDT_ERR_BADLAYOUT   12
    /* FDT_ERR_BADLAYOUT: For read-write functions, the given
     * device tree has it's sub-blocks in an order that the
     * function can't handle (memory reserve map, then structure,
     * then strings).  Use fdt_open_into() to reorganize the tree
     * into a form suitable for the read-write operations. */

/* "Can't happen" error indicating a bug in libfdt */
#define FDT_ERR_INTERNAL    13
    /* FDT_ERR_INTERNAL: libfdt has failed an internal assertion.
     * Should never be returned, if it is, it indicates a bug in
     * libfdt itself. */

/* Errors in device tree content */
#define FDT_ERR_BADNCELLS   14
    /* FDT_ERR_BADNCELLS: Device tree has a #address-cells, #size-cells
     * or similar property with a bad format or value */

#define FDT_ERR_BADVALUE    15
    /* FDT_ERR_BADVALUE: Device tree has a property with an unexpected
     * value. For example: a property expected to contain a string list
     * is not NUL-terminated within the length of its value. */

#define FDT_ERR_BADOVERLAY  16
    /* FDT_ERR_BADOVERLAY: The device tree overlay, while
     * correctly structured, cannot be applied due to some
     * unexpected or missing value, property or node. */

#define FDT_ERR_NOPHANDLES  17
    /* FDT_ERR_NOPHANDLES: The device tree doesn't have any
     * phandle available anymore without causing an overflow */

#define FDT_ERR_BADFLAGS    18
    /* FDT_ERR_BADFLAGS: The function was passed a flags field that
     * contains invalid flags or an invalid combination of flags. */

#define FDT_ERR_MAX         18

struct property {
    char    *name;
    int     length;
    void    *value;

    struct property *next;
};

typedef int (*of_scan_flat_dt_cb)(unsigned long node,
                                  const char *uname,
                                  int depth,
                                  void *data);

void *
__unflatten_device_tree(const void *blob,
                        struct device_node *dad,
                        struct device_node **mynodes,
                        void *(*dt_alloc)(u64 size, u64 align));

int
of_scan_flat_dt(of_scan_flat_dt_cb cb, void *data);

static inline const void *
fdt_offset_ptr_(const void *fdt, int offset)
{
    return (const char *)fdt + fdt_off_dt_struct(fdt) + offset;
}

/* Helper to read a big number; size is in cells (not bytes) */
static inline u64
of_read_number(const u32 *cell, int size)
{
    u64 r = 0;
    for (; size--; cell++)
        r = (r << 32) | be32_to_cpu(*cell);
    return r;
}

extern struct device_node *
of_find_node_opts_by_path(const char *path, const char **opts);

static inline struct device_node *
of_find_node_by_path(const char *path)
{
    return of_find_node_opts_by_path(path, NULL);
}

static inline struct device_node *
of_node_get(struct device_node *node)
{
    return node;
}

static inline void
of_node_put(struct device_node *node)
{
}

int
of_property_read_string(const struct device_node *np,
                        const char *propname,
                        const char **out_string);

static inline bool
is_of_node(const struct fwnode_handle *fwnode)
{
    return !IS_ERR_OR_NULL(fwnode) && fwnode->ops == &of_fwnode_ops;
}

#define to_of_node(__fwnode)                                \
    ({                                                      \
        typeof(__fwnode) __to_of_node_fwnode = (__fwnode);  \
                                                            \
        is_of_node(__to_of_node_fwnode) ?                   \
            container_of(__to_of_node_fwnode,               \
                     struct device_node, fwnode) :          \
            NULL;                                           \
    })

#define of_fwnode_handle(node)                          \
    ({                                                  \
        typeof(node) __of_fwnode_handle_node = (node);  \
                                                        \
        __of_fwnode_handle_node ?                       \
            &__of_fwnode_handle_node->fwnode : NULL;    \
    })

bool
early_init_dt_verify(void);

void
early_init_dt_scan_nodes(void);

void
unflatten_device_tree(void);

#endif /* LIBFDT_H */
