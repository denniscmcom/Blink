// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <stddef.h>

namespace blk
{
struct Allocator;
enum class Result;

/// Representation of a sequence of characters.
///
/// @note We recommend using stack-allocated strings if you know at least the maximum size supported. Only use `String`
/// if you absolutely need a dynamic heap-allocated string.
struct String
{
	/// Pointer to a null-terminated string.
	char* literal;
	/// Character count of the string without the null-termination.
	size_t length;
	/// Total allocated capacity of characters.
	size_t capacity;
	Allocator* allocator;
};

/// Creates a `String` with `literal`. It pre-allocates double the size of the literal string passed.
/// @param literal Should be null-terminated. Its contents are copied, so it does not need to outlive `string`.
Result create_string(String& string, Allocator* allocator, const char* literal);
/// Creates an empty `String`.
Result create_string(String& string, Allocator* allocator);
/// Destroys `string`.
void destroy_string(String& string);
/// Resizes `string` if `string.capacity < capacity`. Trying to resize to a lower `capacity` is a successful no-op.
Result resize(String& string, size_t capacity);
/// Assigns `literal` to an already created `String`.
/// @param literal Should be null-terminated. Its contents are copied, so it does not need to outlive `string`.
Result assign(String& string, const char* literal);
/// Concatenates `rhs` to `lhs` so the resulting `String` is `lhs + rhs + \0`.
Result concat(String& lhs, const String& rhs);
}  // namespace blk
