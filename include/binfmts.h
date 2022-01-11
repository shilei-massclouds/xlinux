/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_BINFMTS_H
#define _LINUX_BINFMTS_H

#define MAX_ARG_STRLEN  (PAGE_SIZE * 32)
#define MAX_ARG_STRINGS 0x7FFFFFFF

/*
 * This structure is used to hold the arguments that are used when loading binaries.
 */
struct linux_binprm {
    struct vm_area_struct *vma;
    unsigned long vma_pages;
    struct mm_struct *mm;
    unsigned long p;        /* current top of mem */

    unsigned long argmin;   /* rlimit marker for copy_strings() */

    int argc, envc;
    const char *filename;   /* Name of binary as seen by procps */
    const char *interp;     /* Name of the binary really executed. */
    const char *fdpath;     /* generated filename for execveat */

    unsigned long loader, exec;

    struct file *file;
};

#endif /* _LINUX_BINFMTS_H */
