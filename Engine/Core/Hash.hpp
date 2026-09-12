// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

#include <stdint.h>
#include <string.h>

namespace blk
{
// TODO: We should research about recommended hashing functions for open addressing hash map.
// A non-uniform distribution increases the number of collisions and the cost of resolving them. For open addressing,
// the hash function should also avoid *runs* of two or more keys to consecutive slots. Such runs may cause the lookup
// cost to skyrocket, even if the load factor is low and collisions are infrequent.
// https://en.wikipedia.org/wiki/Hash_table
// https://en.wikipedia.org/wiki/Primary_clustering

/// State of `Hash_Map` slots used by the linear probing method to resolve conflicting hashes.
enum class Hash_Map_Slot_State : uint8_t
{
	/// Slot is empty and could be used when inserting new elements.
	EMPTY,
	/// Slot is used by a live element.
	USED,
	/// Slot was lazy removed.
	REMOVED,
};

/// Hash map using FNV-1a as the hashing function and open addressing with linear probing to handle collisions. It uses
/// a lazy deletion strategy in which a key–value pair is removed by setting `Hash_Map::states` to
/// `Hash_Map_Slot_State::REMOVED`.
///
/// @warning `Key` should implement `uint64_t hash_fnv1a(const Key& key)` and overload the equality comparison
/// operator.
template <typename Key, typename Value>
struct Hash_Map
{
	/// It is calculated as `(count + removed_count) / capacity`. With open addressing, it cannot be greater than 1 and
	/// recommended range is around 0.6 to 0.75. Values closer to 0 increase performance at the cost of memory
	/// footprint. Values closer to 1 decrease memory footprint at the cost of performance.
	float max_load_factor;

	// We use a SoA layout to store slots.

	/// Unique keys of key-value association.
	Key* keys;
	/// Values of key-value association.
	Value* values;
	/// The state of each slot.
	Hash_Map_Slot_State* states;

	/// Total capacity of `keys`, `values`, and `states`.
	size_t capacity;
	/// The number of currently `Hash_Map_State::USED` slots.
	size_t count;
	/// The number of `Hash_Map_State::REMOVED` slots.
	size_t removed_count;
	Allocator* allocator;
};

/// Creates a `Hash_Map` with `capacity` slots and `max_load_factor`.
///
/// @param max_load_factor It should be greater than 0 and less than 1. 0.6 <= `max_load_factor` <= 0.75 is recommended.
template <typename Key, typename Value>
Result create_hash_map(Hash_Map<Key, Value>& hash_map, Allocator* allocator, size_t capacity, float max_load_factor);
/// Destroys `hash_map`.
template <typename Key, typename Value>
void destroy_hash_map(Hash_Map<Key, Value>& hash_map);
/// Inserts the association `key`–`value` into `hash_map`. If `key` already exists, it replaces `value`.
template <typename Key, typename Value>
void insert(Hash_Map<Key, Value>& hash_map, Key key, Value value);
/// Lazy removes the `key–value` pair from `hash_map`.
template <typename Key, typename Value>
void remove(Hash_Map<Key, Value>& hash_map, Key key);
/// Gets the `value` of the `key–value` pair from `hash_map`.
template <typename Key, typename Value>
Value* get(Hash_Map<Key, Value>& hash_map, Key key);
/// Resize `hash_map` to `capacity` if `capacity` > `hash_map.capacity`.
template <typename Key, typename Value>
Result resize(Hash_Map<Key, Value>& hash_map, size_t capacity);
/// Checks if `key` is in `hash_map`.
template <typename Key, typename Value>
bool contains(const Hash_Map<Key, Value>& hash_map, Key key);

/// Hashes `size` bytes in `buffer` using the FNV-1a function.
uint64_t hash_fnv1a(const char* buffer, size_t size);
/// Hashes `string` using the FNV-1a function.
/// @warning `string` should be null-terminated.
uint64_t hash_fnv1a(const char* string);
/// Hashes a 64 bits unsigned int.
uint64_t hash_fnv1a(uint64_t value);
}  // namespace blk

namespace blk
{
template <typename Key, typename Value>
Result
create_hash_map(Hash_Map<Key, Value>& hash_map, Allocator* allocator, size_t capacity, float max_load_factor)
{
	if (!BLK_VERIFY(allocator) || !BLK_VERIFY(max_load_factor > 0) || !BLK_VERIFY(max_load_factor < 1))
	{
		return Result::INVALID_ARGUMENTS;
	}

	hash_map = {};

	hash_map.allocator = allocator;
	hash_map.max_load_factor = max_load_factor;

	return resize(hash_map, capacity);
}

template <typename Key, typename Value>
void
destroy_hash_map(Hash_Map<Key, Value>& hash_map)
{
	if (hash_map.keys || hash_map.values || hash_map.states)
	{
		if (BLK_VERIFY(hash_map.allocator))
		{
			free(*hash_map.allocator, hash_map.keys);
			free(*hash_map.allocator, hash_map.values);
			free(*hash_map.allocator, hash_map.states);
		}
	}

	hash_map = {};
}

template <typename Key, typename Value>
void
insert(Hash_Map<Key, Value>& hash_map, Key key, Value value)
{
	// Check if we reach the maximum load factor. Because our lazy deletion strategy, removed slots contribute to the
	// calculation of to the load factor.
	if (hash_map.count + hash_map.removed_count >= hash_map.capacity * hash_map.max_load_factor)
	{
		// We reached the maximum load factor so, we need to resize.

		size_t new_capacity = 0;

		if (hash_map.capacity == 0)
		{
			static constexpr size_t initial_capacity = 10;
			new_capacity = initial_capacity;
		}
		else
		{
			new_capacity = hash_map.capacity * 2;
		}

		// TODO (Consistency): `insert` returns `void` while every other fallible operation in `Core` returns `Result`,
		// so a failed grow can only be reported through `BLK_VERIFY` instead of to the caller. See the note at the end
		// of this function for why it was left as `void`, and decide whether that still holds.

		// A failed resize leaves `hash_map.capacity` untouched, and it could still be 0, so we cannot keep probing.
		if (!BLK_VERIFY(resize(hash_map, new_capacity) == Result::SUCCESS))
		{
			return;
		}
	}

	// Hash the key.
	const uint64_t hash = hash_fnv1a(key);

	// Compute the slot index for the new element. As we already know, `index` could collide with an already used index.
	// Because of that, we will continue searching the adjacent slots – eg. `index + 1`, `index + 2` – treating the
	// array as circular, until we find either an empty slot or a slot whose key is `key`. If we find a removed slot
	// along the way, we remember it as a candidate for insertion but keep probing to ensure `key` does not already
	// exist further along the probe chain.

	// First candidate for insertion. `SIZE_MAX` is a sentinel value that means no removed slot found yet.
	size_t first_removed = SIZE_MAX;
	const size_t start = hash % hash_map.capacity;

	for (size_t i = 0; i < hash_map.capacity; ++i)
	{
		const size_t index = (start + i) % hash_map.capacity;

		if (hash_map.states[index] == Hash_Map_Slot_State::EMPTY)
		{
			// Key definitely isn't in the map. Insert at `first_removed` if we passed one, otherwise insert here.
			const size_t target_index = (first_removed != SIZE_MAX) ? first_removed : index;

			hash_map.keys[target_index] = key;
			hash_map.values[target_index] = value;
			hash_map.states[target_index] = Hash_Map_Slot_State::USED;

			hash_map.count += 1;

			if (first_removed == target_index)
			{
				hash_map.removed_count -= 1;
			}

			return;
		}

		if (hash_map.states[index] == Hash_Map_Slot_State::REMOVED)
		{
			// Remember it, but keep probing to check for duplicates. `key` may exists further down the probing chain.
			if (first_removed == SIZE_MAX)
			{
				first_removed = index;
			}
		}

		if (hash_map.states[index] == Hash_Map_Slot_State::USED)
		{
			// We replace `value` if `key` is equal to the slot key. Essentially, we are replacing the value of an
			// existing key.
			if (hash_map.keys[index] == key)
			{
				hash_map.values[index] = value;

				return;
			}
		}
	}

	// By now, the key–value pair should be inserted so this should be unreachable if `hash_map` is in a good state.
	// However, there are some situations in which this line could be reachable:
	// 1. `resize` fail and there are no `Hash_Map_Slot_State::EMPTY` slots available. This could happen if
	// `hash_map.max_load_factor` is 1.0;
	//
	// I decided to not return `Result` because these situations are not really solvable by the caller. I added
	// `BLK_VERIFY` checks to the previous `resize` calls to avoid silent errors.
	BLK_VERIFY(false);
}

template <typename Key, typename Value>
void
remove(Hash_Map<Key, Value>& hash_map, Key key)
{
	if (hash_map.capacity == 0)
	{
		// There are no slots to probe.
		return;
	}

	// To remove a slot is not sufficient to do so by simply emptying it. This would affect searches for other keys that
	// have a hash value earlier than the emptied slot, but are stored in a position later than the emptied slot. The
	// emptied slot would cause those searches to incorrectly report that the key is not present.
	//
	// We use a lazy deletion strategy in which a slot is removed by setting `hash_map.states` to
	// `Hash_Map_Slot_State::REMOVED`. However, these removed slots will contribute to the load factor of the hash
	// table. With this strategy it is necessary to clean the flags and rehash once the load factor reaches
	// `hash_map.max_load_factor`.

	// Compute the slot index for the new element. As we already know, `index` could collide with an already used index.
	// Because of that, we will continue searching the adjacent slots – eg. `index + 1`, `index + 2` – treating the
	// array as circular, until we find either an empty slot or a slot whose key is `key`.

	// Hash the key.
	const uint64_t hash = hash_fnv1a(key);

	const size_t start = hash % hash_map.capacity;

	for (size_t i = 0; i < hash_map.capacity; ++i)
	{
		const size_t index = (start + i) % hash_map.capacity;

		if (hash_map.states[index] == Hash_Map_Slot_State::USED)
		{
			// If the slot key is equal to `key`, we mark it as deleted.
			if (hash_map.keys[index] == key)
			{
				hash_map.states[index] = Hash_Map_Slot_State::REMOVED;
				hash_map.count -= 1;
				hash_map.removed_count += 1;

				return;
			}
		}

		if (hash_map.states[index] == Hash_Map_Slot_State::EMPTY)
		{
			// We found an empty slot. That means `key` cannot be in `hash_map`, because it would have been placed in
			// this slot in preference to any later slots that have not yet been searched.

			return;
		}
	}
}

template <typename Key, typename Value>
Value*
get(Hash_Map<Key, Value>& hash_map, Key key)
{
	if (hash_map.capacity == 0)
	{
		// There are no slots to probe.
		return nullptr;
	}

	// Hash the key.
	const uint64_t hash = hash_fnv1a(key);

	// Compute the slot index for the new element. As we already know, `index` could collide with an already used index.
	// Because of that, we will continue searching the adjacent slots – eg. `index + 1`, `index + 2` – treating the
	// array as circular, until we find either an empty slot or a slot whose key is `key`.

	const size_t start = hash % hash_map.capacity;

	for (size_t i = 0; i < hash_map.capacity; ++i)
	{
		const size_t index = (start + i) % hash_map.capacity;

		if (hash_map.states[index] == Hash_Map_Slot_State::USED)
		{
			// If the slot key is equal to `key`, we found our value.
			if (hash_map.keys[index] == key)
			{
				return &hash_map.values[index];
			}
		}

		if (hash_map.states[index] == Hash_Map_Slot_State::EMPTY)
		{
			// We found an empty slot. That means `key` cannot be in `hash_map`, because it would have been placed in
			// this slot in preference to any later slots that have not yet been searched.

			return nullptr;
		}
	}

	return nullptr;
}

template <typename Key, typename Value>
Result
resize(Hash_Map<Key, Value>& hash_map, size_t capacity)
{
	if (!BLK_VERIFY(hash_map.allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	if (capacity <= hash_map.capacity)
	{
		// We return `SUCCESS` because `hash_map.capacity` is already larger than the requested `capacity`.
		return Result::SUCCESS;
	}

	// Allocate new pointers with the new capacity.

	void* keys_pointer = nullptr;
	BLK_SUCCESS_OR_RETURN(allocate(*hash_map.allocator, keys_pointer, sizeof(Key) * capacity, alignof(Key)));

	void* values_pointer = nullptr;

	if (const Result result = allocate(*hash_map.allocator, values_pointer, sizeof(Value) * capacity, alignof(Value));
		result != Result::SUCCESS)
	{
		free(*hash_map.allocator, keys_pointer);

		return result;
	}

	void* states_pointer = nullptr;

	if (const Result result = allocate(
			*hash_map.allocator,
			states_pointer,
			sizeof(Hash_Map_Slot_State) * capacity,
			alignof(Hash_Map_Slot_State)
		);
		result != Result::SUCCESS)
	{
		free(*hash_map.allocator, keys_pointer);
		free(*hash_map.allocator, values_pointer);

		return result;
	}

	// Copy old data to the new pointers and rehash.

	Key* old_keys = hash_map.keys;
	Value* old_values = hash_map.values;
	Hash_Map_Slot_State* old_states = hash_map.states;
	const size_t old_capacity = hash_map.capacity;

	hash_map.keys = static_cast<Key*>(keys_pointer);
	hash_map.values = static_cast<Value*>(values_pointer);
	hash_map.states = static_cast<Hash_Map_Slot_State*>(states_pointer);
	hash_map.capacity = capacity;
	hash_map.count = 0;
	hash_map.removed_count = 0;

	for (size_t old_index = 0; old_index < old_capacity; ++old_index)
	{
		if (old_states[old_index] != Hash_Map_Slot_State::USED)
		{
			// Skip `Hash_Map_Slot_State::EMPTY` because its zeroed by default when allocating a new buffer and clean
			// the lazy removed slots – `Hash_Map_Slot_State::REMOVED`.
			continue;
		}

		// After resize, the order of the backing arrays might change, for that we call `insert` directly to insert the
		// old key–value pairs into the new resized hash map.
		// TODO (Performance): We could store a list of the cached hashes to avoid recomputing them.
		// That would require to stop calling `insert` directly and handle the reinsertion here.
		insert(hash_map, old_keys[old_index], old_values[old_index]);
	}

	free(*hash_map.allocator, old_keys);
	free(*hash_map.allocator, old_values);
	free(*hash_map.allocator, old_states);

	return Result::SUCCESS;
}

template <typename Key, typename Value>
bool
contains(const Hash_Map<Key, Value>& hash_map, Key key)
{
	if (hash_map.capacity == 0)
	{
		// There are no slots to probe.
		return false;
	}

	// Hash the key.
	const uint64_t hash = hash_fnv1a(key);

	// Linear probing.

	const size_t start = hash % hash_map.capacity;

	for (size_t i = 0; i < hash_map.capacity; ++i)
	{
		const size_t index = (start + i) % hash_map.capacity;

		if (hash_map.states[index] == Hash_Map_Slot_State::EMPTY)
		{
			// Key definitely isn't in the map.
			return false;
		}

		if (hash_map.states[index] == Hash_Map_Slot_State::REMOVED)
		{
			// Keep probing. `key` may exists further down the probing chain.
			continue;
		}

		if (hash_map.states[index] == Hash_Map_Slot_State::USED)
		{
			// We only found `key` if the slot key matches. Otherwise it is a collision, so we keep probing.
			if (hash_map.keys[index] == key)
			{
				return true;
			}
		}
	}

	return false;
}
}  // namespace blk
