// SPDX-License-Identifier: GPL-2.0-only
#include <fdt.h>
#include <sbi.h>

void *initial_boot_params;

static int
fdt_check_header(const void *fdt)
{
    if (fdt_magic(fdt) != FDT_MAGIC)
        return -FDT_ERR_BADMAGIC;

    return 0;
}

bool
early_init_dt_verify(void *params)
{
    if (fdt_check_header(params))
        return false;

    /* Setup flat device-tree pointer */
    initial_boot_params = params;

    return true;
}

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

/**
 * of_scan_flat_dt - scan flattened tree blob and call callback on each.
 * @it: callback function
 * @data: context data pointer
 *
 * This function is used to scan the flattened device-tree, it is
 * used to extract the memory information at boot before we can
 * unflatten the tree
 */
int of_scan_flat_dt(of_scan_flat_dt_cb cb, void *data)
{
    const void *blob = initial_boot_params;
    const char *pathp;
    int offset, rc = 0, depth = -1;

    if (!blob)
        return 0;

    {
        sbi_printf("fdt_header: magic(%x) off_dt_struct(%x)\n",
                   fdt_magic(blob), fdt_off_dt_struct(blob));
    }

    for (offset = fdt_next_node(blob, -1, &depth);
         offset >= 0 && depth >= 0 && !rc;
         offset = fdt_next_node(blob, offset, &depth)) {

        pathp = fdt_get_name(blob, offset, NULL);
        rc = cb(offset, pathp, depth, data);
    }

    return rc;
}

/**
 * early_init_dt_scan_root - fetch the top level address and size cells
 */
static int
early_init_dt_scan_root(unsigned long node,
                        const char *uname,
                        int depth,
                        void *data)
{
    sbi_printf("%s: node(%u) uname(%s) depth(%d)\n",
               __func__, node, uname, depth);
    return 0;
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
    sbi_printf("%s: node(%u) uname(%s) depth(%d)\n",
               __func__, node, uname, depth);
    return 0;
}

void
early_init_dt_scan_nodes(void)
{
    of_scan_flat_dt(early_init_dt_scan_root, NULL);
}

/*
void
unflatten_device_tree(void)
{
    __unflatten_device_tree(initial_boot_params, NULL, &of_root,
                            early_init_dt_alloc_memory_arch, false);
}
*/
