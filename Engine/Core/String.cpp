// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Core/String.hpp"

#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

#include <string.h>

blk::Result
blk::create_string(String& string, Allocator* allocator, const char* literal)
{
	if (!BLK_VERIFY(allocator) || !BLK_VERIFY(literal))
	{
		return Result::INVALID_ARGUMENTS;
	}

	const size_t length = strlen(literal);

	// We pre-allocate twice the length of `literal`.
	const size_t capacity = (length + 1) * 2;

	string = {};
	// First set `string.allocator` because it is needed by `resize`.
	string.allocator = allocator;

	if (const Result result = resize(string, capacity); result != Result::SUCCESS)
	{
		return result;
	}

	// Copy the initial `literal` to the pre-allocated `string.literal`.
	memcpy(string.literal, literal, length);

	// Set the initial length before the null-terminator.
	string.length = length;

	// Add the null-terminator.
	string.literal[string.length] = '\0';

	return Result::SUCCESS;
}

blk::Result
blk::create_string(String& string, Allocator* allocator)
{
	if (!BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	// 50 is the initial capacity when no literal is passed.
	constexpr size_t capacity = 50;

	string = {};

	// Set `string.allocator` before using `resize`.
	string.allocator = allocator;

	if (const Result result = resize(string, capacity); result != Result::SUCCESS)
	{
		return result;
	}

	return Result::SUCCESS;
}

void
blk::destroy_string(String& string)
{
	if (string.literal)
	{
		if (BLK_VERIFY(string.allocator))
		{
			free(*string.allocator, string.literal);
		}
	}

	string = {};
}

blk::Result
blk::resize(String& string, size_t capacity)
{
	if (!BLK_VERIFY(string.allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	if (capacity <= string.capacity)
	{
		// `string.literal` already fits `capacity`, so we return `Result::SUCCESS`.
		return Result::SUCCESS;
	}

	// Allocate the new pointer.
	void* pointer = nullptr;

	if (const Result result = allocate(*string.allocator, pointer, capacity, alignof(char)); result != Result::SUCCESS)
	{
		return result;
	}

	// Save the old literal to copy it to the new pointer later.
	char* old_literal = string.literal;

	// Assign new pointer and new capacity.
	string.literal = static_cast<char*>(pointer);
	string.capacity = capacity;

	// If `string` held a literal before, then we copy it to the new pointer.
	if (old_literal)
	{
		memcpy(string.literal, old_literal, string.length);

		// And free the old pointer because it is no longer in use.
		free(*string.allocator, old_literal);
	}

	// Add the null-terminator.
	string.literal[string.length] = '\0';

	return Result::SUCCESS;
}

blk::Result
blk::assign(String& string, const char* literal)
{
	if (!BLK_VERIFY(string.allocator) || !BLK_VERIFY(literal))
	{
		return Result::INVALID_ARGUMENTS;
	}

	const size_t length = strlen(literal);

	// If `literal` is lengthier than `string.literal`, we need to resize.
	if (length + 1 > string.capacity)
	{
		// Double the capacity of the literal length plus the null-terminator.
		if (const Result result = resize(string, (length + 1) * 2); result != Result::SUCCESS)
		{
			return result;
		}
	}

	// Copy new `literal` to `string.literal`.
	memcpy(string.literal, literal, length);

	// Assign the new length.
	string.length = length;

	// Add the null-terminator.
	string.literal[string.length] = '\0';

	return Result::SUCCESS;
}

blk::Result
blk::concat(String& lhs, const String& rhs)
{
	if (!BLK_VERIFY(lhs.allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	if (rhs.length == 0)
	{
		// We have nothing to concatenate to `lhs`.
		return Result::SUCCESS;
	}

	// If the resulting concatenated string capacity is less than the two strings concatenated plus the null-terminator,
	// we need to resize.
	if (lhs.capacity < lhs.length + rhs.length + 1)
	{
		// We allocate double the resulting concatenated string length plus the null terminator.
		if (const Result result = resize(lhs, (lhs.length + rhs.length + 1) * 2); result != Result::SUCCESS)
		{
			return result;
		}
	}

	// Copy `rhs` to the end of `lhs`.
	memcpy(lhs.literal + lhs.length, rhs.literal, rhs.length);
	lhs.length += rhs.length;

	// Add the null-terminator.
	lhs.literal[lhs.length] = '\0';

	return Result::SUCCESS;
}
