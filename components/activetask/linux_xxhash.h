/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 14:17:51
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-17 14:17:51
 * @FilePath    : /activetask/components/activetask/linux_xxhash.h
 * @Description : https://github.com/torvalds/linux/blob/master/include/linux/xxhash.h
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _LINUX_XXHASH_H_
#define _LINUX_XXHASH_H_

#include <stddef.h>
#include <string.h>

/*-****************************
 * Simple Hash Functions
 *****************************/

/**
 * xxh32() - calculate the 32-bit hash of the input with a given seed.
 *
 * @input:  The data to hash.
 * @length: The length of the data to hash.
 * @seed:   The seed can be used to alter the result predictably.
 *
 * Speed on Core 2 Duo @ 3 GHz (single thread, SMHasher benchmark) : 5.4 GB/s
 *
 * Return:  The 32-bit hash of the data.
 */
uint32_t xxh32(const void *input, size_t length, uint32_t seed);

/**
 * xxh64() - calculate the 64-bit hash of the input with a given seed.
 *
 * @input:  The data to hash.
 * @length: The length of the data to hash.
 * @seed:   The seed can be used to alter the result predictably.
 *
 * This function runs 2x faster on 64-bit systems, but slower on 32-bit systems.
 *
 * Return:  The 64-bit hash of the data.
 */
uint64_t xxh64(const void *input, size_t length, uint64_t seed);

/**
 * xxhash() - calculate wordsize hash of the input with a given seed
 * @input:  The data to hash.
 * @length: The length of the data to hash.
 * @seed:   The seed can be used to alter the result predictably.
 *
 * If the hash does not need to be comparable between machines with
 * different word sizes, this function will call whichever of xxh32()
 * or xxh64() is faster.
 *
 * Return:  wordsize hash of the data.
 */

static inline unsigned long xxhash(const void *input, size_t length,
				   uint64_t seed)
{
#if BITS_PER_LONG == 64
       return xxh64(input, length, seed);
#else
       return xxh32(input, length, seed);
#endif
}

/*-****************************
 * Streaming Hash Functions
 *****************************/

/*
 * These definitions are only meant to allow allocation of XXH state
 * statically, on stack, or in a struct for example.
 * Do not use members directly.
 */

/**
 * struct xxh32_state - private xxh32 state, do not use members directly
 */
struct xxh32_state {
	uint32_t total_len_32;
	uint32_t large_len;
	uint32_t v1;
	uint32_t v2;
	uint32_t v3;
	uint32_t v4;
	uint32_t mem32[4];
	uint32_t memsize;
};

/**
 * struct xxh32_state - private xxh64 state, do not use members directly
 */
struct xxh64_state {
	uint64_t total_len;
	uint64_t v1;
	uint64_t v2;
	uint64_t v3;
	uint64_t v4;
	uint64_t mem64[4];
	uint32_t memsize;
};

/**
 * xxh32_reset() - reset the xxh32 state to start a new hashing operation
 *
 * @state: The xxh32 state to reset.
 * @seed:  Initialize the hash state with this seed.
 *
 * Call this function on any xxh32_state to prepare for a new hashing operation.
 */
void xxh32_reset(struct xxh32_state *state, uint32_t seed);

/**
 * xxh32_update() - hash the data given and update the xxh32 state
 *
 * @state:  The xxh32 state to update.
 * @input:  The data to hash.
 * @length: The length of the data to hash.
 *
 * After calling xxh32_reset() call xxh32_update() as many times as necessary.
 *
 * Return:  Zero on success, otherwise an error code.
 */
int xxh32_update(struct xxh32_state *state, const void *input, size_t length);

/**
 * xxh32_digest() - produce the current xxh32 hash
 *
 * @state: Produce the current xxh32 hash of this state.
 *
 * A hash value can be produced at any time. It is still possible to continue
 * inserting input into the hash state after a call to xxh32_digest(), and
 * generate new hashes later on, by calling xxh32_digest() again.
 *
 * Return: The xxh32 hash stored in the state.
 */
uint32_t xxh32_digest(const struct xxh32_state *state);

/**
 * xxh64_reset() - reset the xxh64 state to start a new hashing operation
 *
 * @state: The xxh64 state to reset.
 * @seed:  Initialize the hash state with this seed.
 */
void xxh64_reset(struct xxh64_state *state, uint64_t seed);

/**
 * xxh64_update() - hash the data given and update the xxh64 state
 * @state:  The xxh64 state to update.
 * @input:  The data to hash.
 * @length: The length of the data to hash.
 *
 * After calling xxh64_reset() call xxh64_update() as many times as necessary.
 *
 * Return:  Zero on success, otherwise an error code.
 */
int xxh64_update(struct xxh64_state *state, const void *input, size_t length);

/**
 * xxh64_digest() - produce the current xxh64 hash
 *
 * @state: Produce the current xxh64 hash of this state.
 *
 * A hash value can be produced at any time. It is still possible to continue
 * inserting input into the hash state after a call to xxh64_digest(), and
 * generate new hashes later on, by calling xxh64_digest() again.
 *
 * Return: The xxh64 hash stored in the state.
 */
uint64_t xxh64_digest(const struct xxh64_state *state);

/*-**************************
 * Utils
 ***************************/

/**
 * xxh32_copy_state() - copy the source state into the destination state
 *
 * @src: The source xxh32 state.
 * @dst: The destination xxh32 state.
 */
void xxh32_copy_state(struct xxh32_state *dst, const struct xxh32_state *src);

/**
 * xxh64_copy_state() - copy the source state into the destination state
 *
 * @src: The source xxh64 state.
 * @dst: The destination xxh64 state.
 */
void xxh64_copy_state(struct xxh64_state *dst, const struct xxh64_state *src);

#endif