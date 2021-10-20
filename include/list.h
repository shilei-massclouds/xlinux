/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LIST_H_
#define _LIST_H_

#include <types.h>

#define offsetof(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({          \
    void *__mptr = (void *)(ptr);                   \
    ((type *)(__mptr - offsetof(type, member))); })

#define __container_of(ptr, sample, member)         \
    (void *)container_of((ptr), typeof(*(sample)), member)

/**
 * list_entry - get the struct for this entry
 * @ptr:    the &struct list_head pointer.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_head within the struct.
 */
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

/**
 * Loop through the list given by head and set pos to struct in the list.
 *
 * Example:
 * struct foo *iterator;
 * list_for_each_entry(iterator, &bar->list_of_foos, entry) {
 *      [modify iterator]
 * }
 *
 * This macro is not safe for node deletion. Use list_for_each_entry_safe
 * instead.
 *
 * @param pos Iterator variable of the type of the list elements.
 * @param head List head
 * @param member Member name of the struct list_head in the list elements.
 *
 */
#define list_for_each_entry(pos, head, member)              \
    for (pos = __container_of((head)->next, pos, member);   \
         &pos->member != (head);                            \
         pos = __container_of(pos->member.next, pos, member))

/**
 * Loop through the list, keeping a backup pointer to the element.
 * This macro allows for the deletion of a list element
 * while looping through the list.
 *
 * See list_for_each_entry for more details.
 */
#define list_for_each_entry_safe(pos, tmp, head, member)        \
    for (pos = __container_of((head)->next, pos, member),       \
         tmp = __container_of(pos->member.next, pos, member);   \
         &pos->member != (head);                                \
         pos = tmp, tmp = __container_of(pos->member.next, tmp, member))

/**
 * list_first_entry_or_null - get the first element from a list
 * @ptr:    the list head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_head within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define list_first_entry_or_null(ptr, type, member) ({  \
    struct list_head *head__ = (ptr);                   \
    struct list_head *pos__ = head__->next;             \
    pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
})

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

static inline void
INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void
__list_add(struct list_head *new,
           struct list_head *prev,
           struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void
list_add(struct list_head *new,
         struct list_head *head)
{
    __list_add(new, head, head->next);
}

static inline void
list_add_tail(struct list_head *new,
              struct list_head *head)
{
    __list_add(new, head->prev, head);
}

static inline void
__list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void
__list_del_entry(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline void
list_del(struct list_head *entry)
{
    __list_del_entry(entry);
    entry->next = NULL;
    entry->prev = NULL;
}

static inline int
list_empty(const struct list_head *head)
{
    return (head->next == head);
}

static inline void
__list_splice(const struct list_head *list,
              struct list_head *prev,
              struct list_head *next)
{
    struct list_head *first = list->next;
    struct list_head *last = list->prev;

    first->prev = prev;
    prev->next = first;

    last->next = next;
    next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void
list_splice(const struct list_head *list, struct list_head *head)
{
    if (!list_empty(list))
        __list_splice(list, head, head->next);
}

#endif /* _LIST_H_ */
