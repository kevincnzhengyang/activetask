/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 11:08:17
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-17 11:08:17
 * @FilePath    : /activetask/components/activetask/linux_llist.h
 * @Description : https://github.com/torvalds/linux/blob/master/include/linux/llist.h
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _LINUX_LLIST_H_
#define _LINUX_LLIST_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "linux_container_of.h"

#ifdef __cplusplus
extern "C" {
#endif

struct llist_head {
	struct llist_node *first;
};

struct llist_node {
	struct llist_node *next;
};

#define LLIST_HEAD_INIT(name)	{ NULL }
#define LLIST_HEAD(name)	struct llist_head name = LLIST_HEAD_INIT(name)

/**
 * init_llist_head - initialize lock-less list head
 * @head:	the head for your lock-less list
 */
static inline void init_llist_head(struct llist_head *list)
{
	list->first = NULL;
}

/**
 * llist_entry - get the struct of this entry
 * @ptr:	the &struct llist_node pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the llist_node within the struct.
 */
#define llist_entry(ptr, type, member)		\
	container_of(ptr, type, member)

/**
 * member_address_is_nonnull - check whether the member address is not NULL
 * @ptr:	the object pointer (struct type * that contains the llist_node)
 * @member:	the name of the llist_node within the struct.
 *
 * This macro is conceptually the same as
 *	&ptr->member != NULL
 * but it works around the fact that compilers can decide that taking a member
 * address is never a NULL pointer.
 *
 * Real objects that start at a high address and have a member at NULL are
 * unlikely to exist, but such pointers may be returned e.g. by the
 * container_of() macro.
 */
#define member_address_is_nonnull(ptr, member)	\
	((uintptr_t)(ptr) + offsetof(typeof(*(ptr)), member) != 0)

/**
 * llist_for_each - iterate over some deleted entries of a lock-less list
 * @pos:	the &struct llist_node to use as a loop cursor
 * @node:	the first entry of deleted list entries
 *
 * In general, some entries of the lock-less list can be traversed
 * safely only after being deleted from list, so start with an entry
 * instead of list head.
 *
 * If being used on entries deleted from lock-less list directly, the
 * traverse order is from the newest to the oldest added entry.  If
 * you want to traverse from the oldest to the newest, you must
 * reverse the order by yourself before traversing.
 */
#define llist_for_each(pos, node)			\
	for ((pos) = (node); pos; (pos) = (pos)->next)

/**
 * llist_for_each_safe - iterate over some deleted entries of a lock-less list
 *			 safe against removal of list entry
 * @pos:	the &struct llist_node to use as a loop cursor
 * @n:		another &struct llist_node to use as temporary storage
 * @node:	the first entry of deleted list entries
 *
 * In general, some entries of the lock-less list can be traversed
 * safely only after being deleted from list, so start with an entry
 * instead of list head.
 *
 * If being used on entries deleted from lock-less list directly, the
 * traverse order is from the newest to the oldest added entry.  If
 * you want to traverse from the oldest to the newest, you must
 * reverse the order by yourself before traversing.
 */
#define llist_for_each_safe(pos, n, node)			\
	for ((pos) = (node); (pos) && ((n) = (pos)->next, true); (pos) = (n))

/**
 * llist_for_each_entry - iterate over some deleted entries of lock-less list of given type
 * @pos:	the type * to use as a loop cursor.
 * @node:	the fist entry of deleted list entries.
 * @member:	the name of the llist_node with the struct.
 *
 * In general, some entries of the lock-less list can be traversed
 * safely only after being removed from list, so start with an entry
 * instead of list head.
 *
 * If being used on entries deleted from lock-less list directly, the
 * traverse order is from the newest to the oldest added entry.  If
 * you want to traverse from the oldest to the newest, you must
 * reverse the order by yourself before traversing.
 */
#define llist_for_each_entry(pos, node, member)				\
	for ((pos) = llist_entry((node), typeof(*(pos)), member);	\
	     member_address_is_nonnull(pos, member);			\
	     (pos) = llist_entry((pos)->member.next, typeof(*(pos)), member))

/**
 * llist_for_each_entry_safe - iterate over some deleted entries of lock-less list of given type
 *			       safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @node:	the first entry of deleted list entries.
 * @member:	the name of the llist_node with the struct.
 *
 * In general, some entries of the lock-less list can be traversed
 * safely only after being removed from list, so start with an entry
 * instead of list head.
 *
 * If being used on entries deleted from lock-less list directly, the
 * traverse order is from the newest to the oldest added entry.  If
 * you want to traverse from the oldest to the newest, you must
 * reverse the order by yourself before traversing.
 */
#define llist_for_each_entry_safe(pos, n, node, member)			       \
	for (pos = llist_entry((node), typeof(*pos), member);		       \
	     member_address_is_nonnull(pos, member) &&			       \
	        (n = llist_entry(pos->member.next, typeof(*n), member), true); \
	     pos = n)

/**
 * llist_empty - tests whether a lock-less list is empty
 * @head:	the list to test
 *
 * Not guaranteed to be accurate or up to date.  Just a quick way to
 * test whether the list is empty without deleting something from the
 * list.
 */
static inline bool llist_empty(const struct llist_head *head)
{
	return head->first == NULL;     //READ_ONCE(head->first) == NULL;
}

static inline struct llist_node *llist_next(struct llist_node *node)
{
	return node->next;
}

/**
 * llist_add_batch - add several linked entries in batch
 * @new_first:	first entry in batch to be added
 * @new_last:	last entry in batch to be added
 * @head:	the head for your lock-less list
 *
 * Return whether list is empty before adding.
 */
static inline bool llist_add_batch(struct llist_node *new_first,
                        struct llist_node *new_last,
                        struct llist_head *head)
{
	struct llist_node *first;

	do {
		new_last->next = first = head->first;   //READ_ONCE(head->first);
	} while (!atomic_compare_exchange_strong(&head->first, &first, new_first));

	return !first;
}

static inline bool __llist_add_batch(struct llist_node *new_first,
                        struct llist_node *new_last,
                        struct llist_head *head)
{
	new_last->next = head->first;
	head->first = new_first;
	return new_last->next == NULL;
}

/**
 * llist_add - add a new entry
 * @new:	new entry to be added
 * @head:	the head for your lock-less list
 *
 * Returns true if the list was empty prior to adding this entry.
 */
static inline bool llist_add(struct llist_node *new, struct llist_head *head)
{
	return llist_add_batch(new, new, head);
}

static inline bool __llist_add(struct llist_node *new, struct llist_head *head)
{
	return __llist_add_batch(new, new, head);
}

/**
 * llist_del_all - delete all entries from lock-less list
 * @head:	the head of lock-less list to delete all entries
 *
 * If list is empty, return NULL, otherwise, delete all entries and
 * return the pointer to the first entry.  The order of entries
 * deleted is from the newest to the oldest added one.
 */
// static inline struct llist_node *llist_del_all(struct llist_head *head)
// {
// 	return xchg(&head->first, NULL);
// }

static inline struct llist_node *__llist_del_all(struct llist_head *head)
{
	struct llist_node *first = head->first;

	head->first = NULL;
	return first;
}

/**
 * llist_del_first - delete the first entry of lock-less list
 * @head:	the head for your lock-less list
 *
 * If list is empty, return NULL, otherwise, return the first entry
 * deleted, this is the newest added one.
 *
 * Only one llist_del_first user can be used simultaneously with
 * multiple llist_add users without lock.  Because otherwise
 * llist_del_first, llist_add, llist_add (or llist_del_all, llist_add,
 * llist_add) sequence in another user may change @head->first->next,
 * but keep @head->first.  If multiple consumers are needed, please
 * use llist_del_all or use lock between consumers.
 */
static inline struct llist_node *llist_del_first(struct llist_head *head)
{
	struct llist_node *entry, *next;

	entry = atomic_load(&head->first);
	do {
		if (entry == NULL)
			return NULL;
		next = entry->next;     //READ_ONCE(entry->next);
	} while (!atomic_compare_exchange_weak(&head->first, &entry, next));

	return entry;
}

/**
 * llist_reverse_order - reverse order of a llist chain
 * @head:	first item of the list to be reversed
 *
 * Reverse the order of a chain of llist entries and return the
 * new first entry.
 */
static inline struct llist_node *llist_reverse_order(struct llist_node *head)
{
	struct llist_node *new_head = NULL;

	while (head) {
		struct llist_node *tmp = head;
		head = head->next;
		tmp->next = new_head;
		new_head = tmp;
	}

	return new_head;
}

#ifdef __cplusplus
}
#endif

#endif /* _LINUX_LLIST_H_ */