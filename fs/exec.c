// SPDX-License-Identifier: GPL-2.0-only

#include <bug.h>
#include <export.h>

int kernel_execve(const char *kernel_filename,
                  const char *const *argv, const char *const *envp)
{
    panic("%s: kernel_filename(%s)!", __func__, kernel_filename);
}
EXPORT_SYMBOL(kernel_execve);
