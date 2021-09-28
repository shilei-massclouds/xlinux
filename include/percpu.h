/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PERCPU_H_
#define __PERCPU_H_
#include <types.h>

void *
__alloc_percpu(size_t size, size_t align);

#endif /* __PERCPU_H_ */
