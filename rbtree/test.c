// SPDX-License-Identifier: GPL-2.0-only

#include <printk.h>
#include <rbtree.h>
#include <string.h>

struct test_node {
    struct rb_node node;
    unsigned int value;
};

static struct test_node *
_search(struct rb_root *root, unsigned int value)
{
    struct rb_node *node = root->rb_node;

    while (node) {
        struct test_node *this = rb_entry(node, struct test_node, node);

        if (value < this->value)
            node = node->rb_left;
        else if (value > this->value)
            node = node->rb_right;
        else
            return this;
    }

    return NULL;
}

static int
_insert(struct test_node *data, struct rb_root *root)
{
    struct rb_node *parent = NULL;
    struct rb_node **link = &(root->rb_node);

    while (*link) {
        struct test_node *this = rb_entry(*link, struct test_node, node);

        parent = *link;
        if (data->value < this->value)
            link = &((*link)->rb_left);
        else if (data->value > this->value)
            link = &((*link)->rb_right);
        else
            return -1;
    }

    rb_link_node(&data->node, parent, link);
    rb_insert_color(&data->node, root);
    return 0;
}

static int
test_insertion(struct rb_root *root)
{
    int i;
    static struct test_node nodes[10];

    for (i = 0; i < 10; i++) {
        nodes[i].value = i;
        if (_insert(&nodes[i], root))
            return -1;
    }

    return 0;
}

static int
test_search(struct rb_root *root)
{
    if (_search(root, 5) == NULL)
        return -1;

    if (_search(root, 10) != NULL)
        return -1;

    return 0;
}

static int
test_deleting(struct rb_root *root)
{
    struct test_node *node;
    node = _search(root, 3);
    if (node == NULL)
        return -1;

    rb_erase(&(node->node), root);

    if (_search(root, 3) != NULL)
        return -1;

    return 0;
}

static int
init_module(void)
{
    struct rb_root rbtree = RB_ROOT;

    printk("module[test_rbtree]: init begin ...\n");

    if (RB_EMPTY_ROOT(&rbtree))
        printk(_GREEN("rbtree is empty!\n"));
    else
        printk(_RED("rbtree is NOT empty!\n"));

    if (test_insertion(&rbtree))
        printk(_RED("rbtree insert failed!\n"));
    else
        printk(_GREEN("rbtree insert ok!\n"));

    if (!RB_EMPTY_ROOT(&rbtree))
        printk(_GREEN("rbtree has nodes!\n"));
    else
        printk(_RED("rbtree is empty!\n"));

    if (test_search(&rbtree))
        printk(_RED("rbtree search failed!\n"));
    else
        printk(_GREEN("rbtree search ok!\n"));

    if (test_deleting(&rbtree))
        printk(_RED("rbtree delete node failed!\n"));
    else
        printk(_GREEN("rbtree delete node ok!\n"));

    printk("module[test_rbtree]: init end!\n");

    return 0;
}
