/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 13:57:10
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-17 13:57:10
 * @FilePath    : /activetask/components/activetask/linux_refcount.h
 * @Description : https://github.com/torvalds/linux/blob/master/include/linux/refcount.h
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _LINUX_REFCOUNT_H_
#define _LINUX_REFCOUNT_H_

#include <stddef.h>
#include <stdatomic.h>

#include "freertos/semphr.h"
#include "freertos/portmacro.h"

/**
 * typedef refcount_t - variant of atomic_t specialized for reference counts
 * @refs: atomic_t counter field
 *
 * The counter saturates at REFCOUNT_SATURATED and will not move once
 * there. This avoids wrapping the counter and causing 'spurious'
 * use-after-free bugs.
 */
typedef struct refcount_struct {
	atomic_int refs;
} refcount_t;

#define REFCOUNT_INIT(n)	{ .refs = ATOMIC_VAR_INIT(n), }
#define REFCOUNT_MAX		INT_MAX
#define REFCOUNT_SATURATED	(INT_MIN / 2)

/**
 * refcount_set - set a refcount's value
 * @r: the refcount
 * @n: value to which the refcount will be set
 */
static inline void refcount_set(refcount_t *r, int n)
{
	atomic_store(&r->refs, n);     //atomic_set(&r->refs, n);
}

/**
 * refcount_read - get a refcount's value
 * @r: the refcount
 *
 * Return: the refcount's value
 */
static inline unsigned int refcount_read(const refcount_t *r)
{
	return (unsigned int)atomic_load(&r->refs);   //atomic_read(&r->refs);
}

static inline bool __refcount_add_not_zero(int i, refcount_t *r, int *oldp)
{
	int old = refcount_read(r);

	do {
		if (!old)
			break;
	// } while (!atomic_try_cmpxchg_relaxed(&r->refs, &old, old + i));
    } while (!atomic_compare_exchange_strong(&r->refs, &old, old + i));

	if (oldp)
		*oldp = old;

	// if (unlikely(old < 0 || old + i < 0))
	// 	refcount_warn_saturate(r, REFCOUNT_ADD_NOT_ZERO_OVF);

	return old;
}

/**
 * refcount_add_not_zero - add a value to a refcount unless it is 0
 * @i: the value to add to the refcount
 * @r: the refcount
 *
 * Will saturate at REFCOUNT_SATURATED and WARN.
 *
 * Provides no memory ordering, it is assumed the caller has guaranteed the
 * object memory to be stable (RCU, etc.). It does provide a control dependency
 * and thereby orders future stores. See the comment on top.
 *
 * Use of this function is not recommended for the normal reference counting
 * use case in which references are taken and released one at a time.  In these
 * cases, refcount_inc(), or one of its variants, should instead be used to
 * increment a reference count.
 *
 * Return: false if the passed refcount is 0, true otherwise
 */
static inline bool refcount_add_not_zero(int i, refcount_t *r)
{
	return __refcount_add_not_zero(i, r, NULL);
}

static inline void __refcount_add(int i, refcount_t *r, int *oldp)
{
	int old = atomic_fetch_add(&r->refs, i);    //atomic_fetch_add_relaxed(i, &r->refs);

	if (oldp)
		*oldp = old;

	// if (unlikely(!old))
	// 	refcount_warn_saturate(r, REFCOUNT_ADD_UAF);
	// else if (unlikely(old < 0 || old + i < 0))
	// 	refcount_warn_saturate(r, REFCOUNT_ADD_OVF);
}

/**
 * refcount_add - add a value to a refcount
 * @i: the value to add to the refcount
 * @r: the refcount
 *
 * Similar to atomic_add(), but will saturate at REFCOUNT_SATURATED and WARN.
 *
 * Provides no memory ordering, it is assumed the caller has guaranteed the
 * object memory to be stable (RCU, etc.). It does provide a control dependency
 * and thereby orders future stores. See the comment on top.
 *
 * Use of this function is not recommended for the normal reference counting
 * use case in which references are taken and released one at a time.  In these
 * cases, refcount_inc(), or one of its variants, should instead be used to
 * increment a reference count.
 */
static inline void refcount_add(int i, refcount_t *r)
{
	__refcount_add(i, r, NULL);
}

static inline bool __refcount_inc_not_zero(refcount_t *r, int *oldp)
{
	return __refcount_add_not_zero(1, r, oldp);
}

/**
 * refcount_inc_not_zero - increment a refcount unless it is 0
 * @r: the refcount to increment
 *
 * Similar to atomic_inc_not_zero(), but will saturate at REFCOUNT_SATURATED
 * and WARN.
 *
 * Provides no memory ordering, it is assumed the caller has guaranteed the
 * object memory to be stable (RCU, etc.). It does provide a control dependency
 * and thereby orders future stores. See the comment on top.
 *
 * Return: true if the increment was successful, false otherwise
 */
static inline bool refcount_inc_not_zero(refcount_t *r)
{
	return __refcount_inc_not_zero(r, NULL);
}

static inline void __refcount_inc(refcount_t *r, int *oldp)
{
	__refcount_add(1, r, oldp);
}

/**
 * refcount_inc - increment a refcount
 * @r: the refcount to increment
 *
 * Similar to atomic_inc(), but will saturate at REFCOUNT_SATURATED and WARN.
 *
 * Provides no memory ordering, it is assumed the caller already has a
 * reference on the object.
 *
 * Will WARN if the refcount is 0, as this represents a possible use-after-free
 * condition.
 */
static inline void refcount_inc(refcount_t *r)
{
	__refcount_inc(r, NULL);
}

static inline bool __refcount_sub_and_test(int i, refcount_t *r, int *oldp)
{
	int old = atomic_fetch_sub(&r->refs, i);    //atomic_fetch_sub_release(i, &r->refs);

	if (oldp)
		*oldp = old;

	if (old == i) {
		// smp_acquire__after_ctrl_dep();
		return true;
	}

	// if (unlikely(old < 0 || old - i < 0))
	// 	refcount_warn_saturate(r, REFCOUNT_SUB_UAF);

	return false;
}

/**
 * refcount_sub_and_test - subtract from a refcount and test if it is 0
 * @i: amount to subtract from the refcount
 * @r: the refcount
 *
 * Similar to atomic_dec_and_test(), but it will WARN, return false and
 * ultimately leak on underflow and will fail to decrement when saturated
 * at REFCOUNT_SATURATED.
 *
 * Provides release memory ordering, such that prior loads and stores are done
 * before, and provides an acquire ordering on success such that free()
 * must come after.
 *
 * Use of this function is not recommended for the normal reference counting
 * use case in which references are taken and released one at a time.  In these
 * cases, refcount_dec(), or one of its variants, should instead be used to
 * decrement a reference count.
 *
 * Return: true if the resulting refcount is 0, false otherwise
 */
static inline bool refcount_sub_and_test(int i, refcount_t *r)
{
	return __refcount_sub_and_test(i, r, NULL);
}

static inline bool __refcount_dec_and_test(refcount_t *r, int *oldp)
{
	return __refcount_sub_and_test(1, r, oldp);
}

/**
 * refcount_dec_and_test - decrement a refcount and test if it is 0
 * @r: the refcount
 *
 * Similar to atomic_dec_and_test(), it will WARN on underflow and fail to
 * decrement when saturated at REFCOUNT_SATURATED.
 *
 * Provides release memory ordering, such that prior loads and stores are done
 * before, and provides an acquire ordering on success such that free()
 * must come after.
 *
 * Return: true if the resulting refcount is 0, false otherwise
 */
static inline bool refcount_dec_and_test(refcount_t *r)
{
	return __refcount_dec_and_test(r, NULL);
}

static inline void __refcount_dec(refcount_t *r, int *oldp)
{
	int old = atomic_fetch_sub(&r->refs, 1);    //atomic_fetch_sub_release(1, &r->refs);

	if (oldp)
		*oldp = old;

	// if (unlikely(old <= 1))
	// 	refcount_warn_saturate(r, REFCOUNT_DEC_LEAK);
}

/**
 * refcount_dec - decrement a refcount
 * @r: the refcount
 *
 * Similar to atomic_dec(), it will WARN on underflow and fail to decrement
 * when saturated at REFCOUNT_SATURATED.
 *
 * Provides release memory ordering, such that prior loads and stores are done
 * before.
 */
static inline void refcount_dec(refcount_t *r)
{
	__refcount_dec(r, NULL);
}

/**
 * refcount_dec_if_one - decrement a refcount if it is 1
 * @r: the refcount
 *
 * No atomic_t counterpart, it attempts a 1 -> 0 transition and returns the
 * success thereof.
 *
 * Like all decrement operations, it provides release memory order and provides
 * a control dependency.
 *
 * It can be used like a try-delete operator; this explicit case is provided
 * and not cmpxchg in generic, because that would allow implementing unsafe
 * operations.
 *
 * Return: true if the resulting refcount is 0, false otherwise
 */
static bool refcount_dec_if_one(refcount_t *r)
{
	int val = 1;

	// return atomic_try_cmpxchg_release(&r->refs, &val, 0);
    return atomic_compare_exchange_weak(&r->refs, &val, 0);
}

/**
 * refcount_dec_not_one - decrement a refcount if it is not 1
 * @r: the refcount
 *
 * No atomic_t counterpart, it decrements unless the value is 1, in which case
 * it will return false.
 *
 * Was often done like: atomic_add_unless(&var, -1, 1)
 *
 * Return: true if the decrement operation was successful, false otherwise
 */
bool refcount_dec_not_one(refcount_t *r)
{
	unsigned int new, val = atomic_load(&r->refs);  //atomic_read(&r->refs);

	do {
		// if (unlikely(val == REFCOUNT_SATURATED))
        if (val == REFCOUNT_SATURATED)
			return true;

		if (val == 1)
			return false;

		new = val - 1;
		if (new > val) {
			// WARN_ONCE(new > val, "refcount_t: underflow; use-after-free.\n");
			return true;
		}
    // } while (!atomic_try_cmpxchg_release(&r->refs, &val, new));
	} while (!atomic_compare_exchange_weak(&r->refs, &val, new));

	return true;
}

/**
 * refcount_dec_and_lock - return holding spinlock if able to decrement
 *                         refcount to 0
 * @r: the refcount
 * @lock: the spinlock to be locked
 *
 * Similar to atomic_dec_and_lock(), it will WARN on underflow and fail to
 * decrement when saturated at REFCOUNT_SATURATED.
 *
 * Provides release memory ordering, such that prior loads and stores are done
 * before, and provides a control dependency such that free() must come after.
 * See the comment on top.
 *
 * Return: true and hold spinlock if able to decrement refcount to 0, false
 *         otherwise
 */
bool refcount_dec_and_lock(refcount_t *r, SemaphoreHandle_t lock)
{
	if (refcount_dec_not_one(r))
		return false;

	xSemaphoreTake(lock, portMAX_DELAY);    //spin_lock(lock);
	if (!refcount_dec_and_test(r)) {
		xSemaphoreGive(lock);   //spin_unlock(lock);
		return false;
	}

	return true;
}

/********************************************
 *
 *  kref
 *
 ********************************************
 */

struct kref {
	refcount_t refcount;
};

#define KREF_INIT(n)	{ .refcount = REFCOUNT_INIT(n), }

/**
 * kref_init - initialize object.
 * @kref: object in question.
 */
static inline void kref_init(struct kref *kref)
{
	refcount_set(&kref->refcount, 1);
}

static inline unsigned int kref_read(const struct kref *kref)
{
	return refcount_read(&kref->refcount);
}

/**
 * kref_get - increment refcount for object.
 * @kref: object.
 */
static inline void kref_get(struct kref *kref)
{
	refcount_inc(&kref->refcount);
}

/**
 * kref_put - decrement refcount for object.
 * @kref: object.
 * @release: pointer to the function that will clean up the object when the
 *	     last reference to the object is released.
 *	     This pointer is required, and it is not acceptable to pass kfree
 *	     in as this function.
 *
 * Decrement the refcount, and if 0, call release().
 * Return 1 if the object was removed, otherwise return 0.  Beware, if this
 * function returns 0, you still can not count on the kref from remaining in
 * memory.  Only use the return value if you want to see if the kref is now
 * gone, not present.
 */
static inline int kref_put(struct kref *kref, void (*release)(struct kref *kref))
{
	if (refcount_dec_and_test(&kref->refcount)) {
		release(kref);
		return 1;
	}
	return 0;
}

static inline int kref_put_lock(struct kref *kref,
				void (*release)(struct kref *kref),
				SemaphoreHandle_t lock)
{
	if (refcount_dec_and_lock(&kref->refcount, lock)) {
		release(kref);
		return 1;
	}
	return 0;
}

/**
 * kref_get_unless_zero - Increment refcount for object unless it is zero.
 * @kref: object.
 *
 * Return non-zero if the increment succeeded. Otherwise return 0.
 *
 * This function is intended to simplify locking around refcounting for
 * objects that can be looked up from a lookup structure, and which are
 * removed from that lookup structure in the object destructor.
 * Operations on such objects require at least a read lock around
 * lookup + kref_get, and a write lock around kref_put + remove from lookup
 * structure. Furthermore, RCU implementations become extremely tricky.
 * With a lookup followed by a kref_get_unless_zero *with return value check*
 * locking in the kref_put path can be deferred to the actual removal from
 * the lookup structure and RCU lookups become trivial.
 */
static inline int kref_get_unless_zero(struct kref *kref)
{
	return refcount_inc_not_zero(&kref->refcount);
}
#endif
