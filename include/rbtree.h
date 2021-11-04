/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _LINUX_RBTREE_H_
#define _LINUX_RBTREE_H_

#include <list.h>
#include <atomic.h>

struct rb_node {
    unsigned long  __rb_parent_color;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));

struct rb_root {
    struct rb_node *rb_node;
};

#define RB_RED      0
#define RB_BLACK    1

#define rb_parent(r) ((struct rb_node *)((r)->__rb_parent_color & ~3))

#define __rb_parent(pc)     ((struct rb_node *)(pc & ~3))

#define __rb_color(pc)      ((pc) & 1)
#define __rb_is_red(pc)     (!__rb_color(pc))
#define __rb_is_black(pc)   __rb_color(pc)
#define rb_color(rb)        __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb)       __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb)     __rb_is_black((rb)->__rb_parent_color)

#define RB_ROOT (struct rb_root) { NULL, }

#define RB_EMPTY_ROOT(root) (READ_ONCE((root)->rb_node) == NULL)

#define rb_entry(ptr, type, member) container_of(ptr, type, member)

void rb_insert_color(struct rb_node *, struct rb_root *);
void rb_erase(struct rb_node *, struct rb_root *);

static inline void
rb_link_node(struct rb_node *node,
             struct rb_node *parent,
             struct rb_node **rb_link)
{
    node->__rb_parent_color = (unsigned long)parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
}

static inline void
rb_set_parent_color(struct rb_node *rb, struct rb_node *p, int color)
{
    rb->__rb_parent_color = (unsigned long)p | color;
}

static inline void
__rb_change_child(struct rb_node *old, struct rb_node *new,
                  struct rb_node *parent, struct rb_root *root)
{
    if (parent) {
        if (parent->rb_left == old)
            WRITE_ONCE(parent->rb_left, new);
        else
            WRITE_ONCE(parent->rb_right, new);
    } else {
        WRITE_ONCE(root->rb_node, new);
    }
}

static inline void
rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
    rb->__rb_parent_color = rb_color(rb) | (unsigned long)p;
}

#endif /* _LINUX_RBTREE_H_ */
