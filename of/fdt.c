// SPDX-License-Identifier: GPL-2.0-only
#include <fdt.h>
#include <sbi.h>
#include <types.h>

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
