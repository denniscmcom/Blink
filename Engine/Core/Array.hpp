// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

#include <stddef.h>
#include <string.h>

namespace blk
{
/// Non-owning view into an array.
template <typename Type>
struct Array_View
{
	/// Data buffer.
	Type* buffer;
	/// Number of `Type` in `buffer`.
	size_t count;
};

/// Fixed-size array.
template <typename Type, size_t Capacity>
struct Array
{
	/// Data buffer.
	Type buffer[Capacity];
	/// Capacity of `buffer`.
	static constexpr size_t capacity = Capacity;
};

template <typename Type, size_t Capacity>
Array_View<const Type> to_array_view(const Array<Type, Capacity>& array);

/// Dynamic array.
template <typename Type>
struct Dyn_Array
{
	/// Data buffer.
	Type* buffer;
	/// Number of live `Type`.
	size_t count;
	/// Number of `Type` that fits in `buffer`.
	size_t capacity;
	/// Pointer to `Allocator`.
	Allocator* allocator;
};

/// Creates a dynamic array.
template <typename Type>
Result create_dyn_array(Dyn_Array<Type>& array, Allocator* allocator, size_t capacity);
/// Destroys a dynamic array.
template <typename Type>
void destroy_dyn_array(Dyn_Array<Type>& array);
/// Pushes a new `value` to the end of `array`.
template <typename Type>
void push(Dyn_Array<Type>& array, Type value);
/// Resizes `buffer` if `capacity > array.capacity`.
template <typename Type>
Result resize(Dyn_Array<Type>& array, size_t capacity);
/// Removes all elements from `array`.
template <typename Type>
void empty(Dyn_Array<Type>& array);
/// Inserts a `value` into `array` at `index`.
template <typename Type>
Result insert(Dyn_Array<Type>& array, Type value, size_t index);
}  // namespace blk

namespace blk
{
template <typename Type, size_t Capacity>
Array_View<const Type>
to_array_view(const Array<Type, Capacity>& array)
{
	return Array_View{
		.buffer = array.buffer,
		.count = array.capacity,
	};
}

template <typename Type>
Result
create_dyn_array(Dyn_Array<Type>& array, Allocator* allocator, size_t capacity)
{
	if (!BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	array = {};
	// We have to set `allocator` first because `resize` relies on it.
	array.allocator = allocator;

	if (const Result result = resize(array, capacity); result != Result::SUCCESS)
	{
		return result;
	}

	return Result::SUCCESS;
}

template <typename Type>
void
destroy_dyn_array(Dyn_Array<Type>& array)
{
	if (array.buffer)
	{
		if (!BLK_VERIFY(array.allocator))
		{
			return;
		}

		free(*array.allocator, array.buffer);
	}

	array = {};
}

template <typename Type>
void
push(Dyn_Array<Type>& array, Type value)
{
	if (array.capacity <= array.count)
	{
		// Resize.
		size_t new_capacity = 0;

		if (array.capacity == 0)
		{
			new_capacity = 10;
		}
		else
		{
			new_capacity = array.capacity * 2;
		}

		// TODO (Consistency): `push` returns `void` while every other fallible operation in `Core` returns `Result`, so
		// a failed grow can only be reported through `BLK_VERIFY` instead of to the caller. Decide whether `push`
		// should return `Result` -and update every call site- or stay `void` and treat a failed grow as unrecoverable.
		if (!BLK_VERIFY(resize(array, new_capacity) == Result::SUCCESS))
		{
			return;
		}
	}

	array.buffer[array.count] = value;
	array.count += 1;
}

template <typename Type>
Result
resize(Dyn_Array<Type>& array, size_t capacity)
{
	if (!BLK_VERIFY(array.allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	if (capacity <= array.capacity)
	{
		// `array.buffer` already fits `capacity`, so we return `Result::SUCCESS`.
		return Result::SUCCESS;
	}

	void* pointer = nullptr;

	if (const Result result = allocate(*array.allocator, pointer, sizeof(Type) * capacity, alignof(Type));
		result != Result::SUCCESS)
	{
		return result;
	}

	// On the first `resize` there is no buffer to copy from nor to free.
	if (array.buffer)
	{
		memcpy(pointer, array.buffer, sizeof(Type) * array.count);
		free(*array.allocator, array.buffer);
	}

	array.buffer = static_cast<Type*>(pointer);
	array.capacity = capacity;

	return Result::SUCCESS;
}

template <typename Type>
void
empty(Dyn_Array<Type>& array)
{
	array.count = 0;
}

template <typename Type>
Result
insert(Dyn_Array<Type>& array, Type value, size_t index)
{
	if (array.capacity <= index)
	{
		return Result::OUT_OF_BOUNDS;
	}

	array.buffer[index] = value;

	return Result::SUCCESS;
}
}  // namespace blk
