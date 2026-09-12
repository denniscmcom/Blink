// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

/// Memory allocators.

#pragma once

#include "Engine/Platform/Result.hpp"

namespace blk
{
/// Allocator type.
enum class Allocator_Type
{
	/// Reserves a fixed-size block of memory from the OS once at creation and slices it to handle allocations.
	///
	/// It does not support resizing after creation nor individual frees. Calling `free` is a no-op.
	ARENA,
};

/// Generic allocator.
struct Allocator
{
	/// Opaque pointer to the actual allocator's context.
	void* context;
	/// Type of the allocator pointed to by `context`.
	Allocator_Type type;

	/// Pointer to `allocate` function.
	Result (*allocate)(void* context, void*& pointer, size_t size, size_t alignment);
	/// Pointer to `free` function.
	void (*free)(void* context, void* pointer);
	/// Pointer to `reset` function.
	void (*reset)(void* context);
};

/// Allocates `size` bytes aligned to `alignment` and returns `pointer`. Initializes all bytes to zero.
Result allocate(const Allocator& allocator, void*& pointer, size_t size, size_t alignment);
/// Frees `pointer`.
void free(const Allocator& allocator, void* pointer);
/// Resets the state of the allocator.
void reset(const Allocator& allocator);

/// Creates arena allocator.
Result create_arena_allocator(Allocator& allocator, size_t capacity);
/// Destroys arena allocator.
void destroy_arena_allocator(Allocator& allocator);
}  // namespace blk
