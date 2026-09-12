// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Platform/Allocator.hpp"

#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

#include <stdlib.h>
#include <string.h>

namespace
{
/// Context for arena allocator.
struct Arena_Context
{
	/// Memory buffer.
	char* buffer;
	/// Size of `buffer`.
	size_t capacity;
	/// Next allocation position in `buffer`.
	size_t offset;
};

/// Allocates `size` bytes aligned to `alignment` and returns `pointer` using arena method. Initializes all bytes to
/// zero.
blk::Result arena_allocate(void* context, void*& pointer, size_t size, size_t alignment);
/// No-op.
void arena_free(void* context, void* pointer);
/// Resets `offset` to 0, so old allocations are overwritten by new ones.
void arena_reset(void* context);
}  // namespace

blk::Result
blk::allocate(const Allocator& allocator, void*& pointer, size_t size, size_t alignment)
{
	BLK_CHECK(allocator.allocate);
	BLK_CHECK(allocator.context);

	return allocator.allocate(allocator.context, pointer, size, alignment);
}

void
blk::free(const Allocator& allocator, void* pointer)
{
	BLK_CHECK(allocator.free);
	BLK_CHECK(allocator.context);

	allocator.free(allocator.context, pointer);
}

void
blk::reset(const Allocator& allocator)
{
	BLK_CHECK(allocator.reset);
	BLK_CHECK(allocator.context);

	allocator.reset(allocator.context);
}

blk::Result
blk::create_arena_allocator(Allocator& allocator, const size_t capacity)
{
	allocator = {};
	auto* context = static_cast<Arena_Context*>(calloc(1, sizeof(Arena_Context)));

	if (!BLK_VERIFY(context))
	{
		return Result::OUT_OF_MEMORY;
	}

	context->buffer = static_cast<char*>(calloc(1, capacity));

	if (!BLK_VERIFY(context->buffer))
	{
		::free(context);

		return Result::OUT_OF_MEMORY;
	}

	context->capacity = capacity;

	allocator.context = context;
	allocator.type = Allocator_Type::ARENA;
	allocator.allocate = arena_allocate;
	allocator.free = arena_free;
	allocator.reset = arena_reset;

	return Result::SUCCESS;
}

void
blk::destroy_arena_allocator(Allocator& allocator)
{
	BLK_CHECK(allocator.type == Allocator_Type::ARENA);

	if (!BLK_VERIFY(allocator.context))
	{
		// Allocator could be corrupted.
		return;
	}

	auto* context = static_cast<Arena_Context*>(allocator.context);

	if (!BLK_VERIFY(context->buffer))
	{
		// Allocator could be corrupted.
		::free(context);
		allocator = {};

		return;
	}

	::free(context->buffer);
	::free(context);

	allocator = {};
}

namespace
{
blk::Result
arena_allocate(void* context, void*& pointer, size_t size, size_t alignment)
{
	BLK_CHECK(context);

	auto* arena_context = static_cast<Arena_Context*>(context);

	if (!BLK_VERIFY(arena_context->buffer))
	{
		// Arena context could be corrupted.
		return blk::Result::OUT_OF_MEMORY;
	}

	// The mask only rounds up correctly when `alignment` is a power of two.
	BLK_CHECK(alignment > 0 && (alignment & (alignment - 1)) == 0);

	const size_t aligned_offset = (arena_context->offset + alignment - 1) & ~(alignment - 1);

	if (arena_context->capacity < aligned_offset + size)
	{
		return blk::Result::OUT_OF_MEMORY;
	}

	pointer = arena_context->buffer + aligned_offset;
	arena_context->offset = aligned_offset + size;

	return blk::Result::SUCCESS;
}

void
arena_free(void* /*context*/, void* /*pointer*/)
{
	// No-op.
}

void
arena_reset(void* context)
{
	BLK_CHECK(context);

	auto* arena_context = static_cast<Arena_Context*>(context);

	// The caller relies of the buffer being zeroed when the allocator is created. So, we have to zero it again on
	// reset.
	memset(arena_context->buffer, 0, arena_context->offset);

	arena_context->offset = 0;
}
}  // namespace
