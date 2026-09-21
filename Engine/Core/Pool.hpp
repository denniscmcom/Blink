// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Hash.hpp"
#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace blk
{
/// Handle returned by `Pool` when inserting elements.
///
/// A lightweight, copyable handle that identifies an element in a `Pool`.
/// Handles are versioned: a handle becomes stale after its element is removed, so reusing a handle after removal will
/// not accidentally alias a newer element occupying the same slot.
template <typename>
struct Pool_Handle
{
	/// Element's position in the Pool's buffer.
	/// `SIZE_MAX` is the sentinel value that represents an invalid id.
	size_t id = SIZE_MAX;
	/// Element's slot version.
	/// `SIZE_MAX` is the sentinel value that represents an invalid version.
	size_t version = SIZE_MAX;
};

/// Hashes a `Pool_Handle`.
/// @note It is required to create `Hash_Map<Key, Value>` where `Key` is a `Pool_Handle`.
template <typename Type>
uint64_t hash_fnv1a(const Pool_Handle<Type>& key);

/// Invalid `Pool_Handle`. Compares unequal to every valid handle.
template <typename Type>
constexpr Pool_Handle<Type> POOL_HANDLE_NONE = {.id = SIZE_MAX, .version = SIZE_MAX};

/// Comparison operator for `Pool_Handle`.
/// Two handles are equal when both their `id` and `version` match.
template <typename Type>
bool operator==(const Pool_Handle<Type>& lhs, const Pool_Handle<Type>& rhs);

/// Internal slot used by `Pool` to track per-element occupancy and versioning.
template <typename Type>
struct Pool_Slot
{
	/// The stored element. Garbage when `is_used` is `false`.
	Type element;
	/// Whether this slot currently holds a live element.
	bool is_used;
	/// Monotonically increasing version counter. Compared against `Pool_Handle::version` to detect stale access.
	size_t version;
};

/// Pre-allocated, slot-based pool of `Type`.
///
/// Stores elements of `Type` in a flat, contiguous buffer and tracks occupancy per slot. Insertion reuses the first
/// available slot and returns a versioned handle; removal frees the slot and bumps its version so that stale handles
/// are safely detected on subsequent access.
///
/// Useful when stable, O(1) insert / remove / lookup by handle is needed and want to avoid pointer invalidation issues.
template <typename Type>
struct Pool
{
	/// Buffer of `Pool_Slot`.
	Pool_Slot<Type>* slots;
	/// Total capacity of `slots`.
	size_t capacity;
	/// The number of currently occupied (live) elements in `slots`.
	size_t count;

	/// List of unused positions in `slots`.
	size_t* free_indices;
	/// The number of indices in `free_indices`.
	size_t free_indices_count;
	/// Pointer to allocator.
	Allocator* allocator;
};

/// Creates a `Pool`.
template <typename Type>
Result create_pool(Pool<Type>& pool, Allocator* allocator, size_t capacity);
/// Destroys a `Pool`.
template <typename Type>
void destroy_pool(Pool<Type>& pool);
/// Inserts `element` and returns its handle, or `POOL_HANDLE_NONE` if `pool` could not grow to fit it.
template <typename Type>
Pool_Handle<Type> insert(Pool<Type>& pool, Type element);
/// Removes the element identified by `handle`. If `handle` is stale, `remove` is a no-op.
template <typename Type>
void remove(Pool<Type>& pool, const Pool_Handle<Type>& handle);
/// Resizes `buffer` if `pool.capacity < capacity`. Trying to resize to a lower `capacity` is a successful no-op.
template <typename Type>
Result resize(Pool<Type>& pool, size_t capacity);
/// Returns a pointer to the element identified by `handle`, or `nullptr` if the handle is stale.
template <typename Type>
Type* get(Pool<Type>& pool, const Pool_Handle<Type>& handle);
}  // namespace blk

namespace blk
{
template <typename Type>
uint64_t
hash_fnv1a(const Pool_Handle<Type>& key)
{
	// This works as long as `Pool_Handle` does not change or add fields. Two adjacent members of the same type are
	// guaranteed to have no padding between them, so we can feed both `id` and `version` as one contiguous byte
	// sequence through buffer overload.
	static_assert(sizeof(Pool_Handle<Type>) == sizeof(key.id) + sizeof(key.version));

	return hash_fnv1a(reinterpret_cast<const char*>(&key.id), sizeof(key.id) + sizeof(key.version));
}

template <typename Type>
bool
operator==(const Pool_Handle<Type>& lhs, const Pool_Handle<Type>& rhs)
{
	return lhs.id == rhs.id && lhs.version == rhs.version;
}

template <typename Type>
Result
create_pool(Pool<Type>& pool, Allocator* allocator, size_t capacity)
{
	BLK_CHECK(allocator);

	pool = {};
	pool.allocator = allocator;

	if (const Result result = resize(pool, capacity); result != Result::SUCCESS)
	{
		return result;
	}

	return Result::SUCCESS;
}

template <typename Type>
void
destroy_pool(Pool<Type>& pool)
{
	if (pool.slots)
	{
		if (BLK_VERIFY(pool.allocator))
		{
			free(*pool.allocator, pool.slots);
		}
	}

	if (pool.free_indices)
	{
		if (BLK_VERIFY(pool.allocator))
		{
			free(*pool.allocator, pool.free_indices);
		}
	}

	pool = {};
}

template <typename Type>
Pool_Handle<Type>
insert(Pool<Type>& pool, Type element)
{
	if (pool.free_indices_count == 0)
	{
		size_t new_capacity = 0;

		if (pool.capacity == 0)
		{
			new_capacity = 10;
		}
		else
		{
			new_capacity = pool.capacity * 2;
		}

		if (!BLK_VERIFY(resize(pool, new_capacity) == Result::SUCCESS))
		{
			return POOL_HANDLE_NONE<Type>;
		}
	}

	pool.free_indices_count -= 1;
	const size_t free_index = pool.free_indices[pool.free_indices_count];

	Pool_Slot<Type>& slot = pool.slots[free_index];
	slot.element = element;
	slot.is_used = true;

	pool.count += 1;

	return Pool_Handle<Type>{
		.id = free_index,
		.version = slot.version,
	};
}

template <typename Type>
void
remove(Pool<Type>& pool, const Pool_Handle<Type>& handle)
{
	if (pool.capacity <= handle.id)
	{
		return;
	}

	if (Pool_Slot<Type>& slot = pool.slots[handle.id]; slot.is_used && slot.version == handle.version)
	{
		slot.element = {};
		slot.is_used = false;
		slot.version += 1;

		pool.count -= 1;

		pool.free_indices[pool.free_indices_count] = handle.id;
		pool.free_indices_count += 1;
	}
}

template <typename Type>
Result
resize(Pool<Type>& pool, const size_t capacity)
{
	if (!BLK_VERIFY(pool.allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	if (capacity <= pool.capacity || capacity == 0)
	{
		return Result::SUCCESS;
	}

	// We need `old_capacity` later when filling `pool.free_indices` with the new indices.
	const size_t old_capacity = pool.capacity;

	// Allocate both buffers before touching `pool`, so a failed allocation leaves it in its original state.
	void* slots_pointer = nullptr;

	if (const Result result =
			allocate(*pool.allocator, slots_pointer, sizeof(Pool_Slot<Type>) * capacity, alignof(Pool_Slot<Type>));
		result != Result::SUCCESS)
	{
		return result;
	}

	void* free_indices_pointer = nullptr;

	if (const Result result =
			allocate(*pool.allocator, free_indices_pointer, sizeof(size_t) * capacity, alignof(size_t));
		result != Result::SUCCESS)
	{
		free(*pool.allocator, slots_pointer);

		return result;
	}

	// Only copy to `slots_pointer` if `pool.slots` was already allocated. We copy `pool.capacity` slots and not
	// `pool.count` because `pool.slots` could be fragmented, and because every slot carries a `version` that has to
	// survive the resize to keep stale handles detectable.
	if (pool.capacity > 0)
	{
		memcpy(slots_pointer, pool.slots, sizeof(Pool_Slot<Type>) * pool.capacity);
	}

	// Only copy old `pool.free_indices` if it's not empty.
	if (pool.free_indices_count > 0)
	{
		// Notice we use `pool.free_indices_count` here because `pool.free_indices` is a contiguous array.
		memcpy(free_indices_pointer, pool.free_indices, sizeof(size_t) * pool.free_indices_count);
	}

	// Do not forget to free the old buffers.
	free(*pool.allocator, pool.slots);
	free(*pool.allocator, pool.free_indices);

	// Both allocations succeeded, so we can commit them. We do not assign the new `capacity` to
	// `pool.free_indices_count` here because we'll set it below when filling the list with new indices.
	pool.slots = static_cast<Pool_Slot<Type>*>(slots_pointer);
	pool.free_indices = static_cast<size_t*>(free_indices_pointer);
	pool.capacity = capacity;

	// Finally, fill `pool.free_indices` with the new indices that fit in the new `capacity`.

	for (size_t i = pool.capacity; i > old_capacity; --i)
	{
		// When zero initializing a `Pool_Handle`, the `id` and `version` fields are zero, which are a
		// valid handle. So, `version` start at 1 so a zeroed `Pool_Handle` can never match a live slot.
		pool.slots[i - 1].version = 1;

		pool.free_indices[pool.free_indices_count] = i - 1;
		pool.free_indices_count += 1;
	}

	return Result::SUCCESS;
}

template <typename Type>
Type*
get(Pool<Type>& pool, const Pool_Handle<Type>& handle)
{
	if (pool.capacity <= handle.id)
	{
		return nullptr;
	}

	Pool_Slot<Type>& slot = pool.slots[handle.id];

	if (!slot.is_used || slot.version != handle.version)
	{
		return nullptr;
	}

	return &slot.element;
}
}  // namespace blk
