/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_BINFMTS_H
#define _LINUX_BINFMTS_H

/*
 * This structure is used to hold the arguments that are used when loading binaries.
 */
struct linux_binprm {
    struct mm_struct *mm;

    const char *filename;   /* Name of binary as seen by procps */
    const char *interp;     /* Name of the binary really executed. */
    const char *fdpath;     /* generated filename for execveat */
};

#endif /* _LINUX_BINFMTS_H */