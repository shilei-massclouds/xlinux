/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __LINUX_FILE_H
#define __LINUX_FILE_H

int get_unused_fd_flags(unsigned flags);

void fd_install(unsigned int fd, struct file *file);

#endif /* __LINUX_FILE_H */
