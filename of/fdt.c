// SPDX-License-Identifier: GPL-2.0-only
#include <fdt.h>
#include <sbi.h>

//void *initial_boot_params;

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
    sbi_printf("%s: 1\n", __func__);
    if (fdt_check_header(params))
        return false;

    sbi_printf("%s: 2\n", __func__);
    /* Setup flat device-tree pointer */
    //initial_boot_params = params;
    sbi_printf("%s: 3\n", __func__);

    return true;
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
    /*
int of_scan_flat_dt(of_scan_flat_dt_cb cb, void *data)
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
        rc = it(offset, pathp, depth, data);
    }
    return rc;
}
    */

/*
void
unflatten_device_tree(void)
{
    __unflatten_device_tree(initial_boot_params, NULL, &of_root,
                            early_init_dt_alloc_memory_arch, false);
}
*/
