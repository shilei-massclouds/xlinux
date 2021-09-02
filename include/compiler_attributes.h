/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_COMPILER_ATTRIBUTES_H
#define __LINUX_COMPILER_ATTRIBUTES_H

#define __section(S)            __attribute__((__section__(#S)))
#define __cold                  __attribute__((__cold__))
#define __aligned(x)            __attribute__((__aligned__(x)))
#define __attribute_const__     __attribute__((__const__))
#define __always_inline         inline __attribute__((__always_inline__))

#endif /* __LINUX_COMPILER_ATTRIBUTES_H */
