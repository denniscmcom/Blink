// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Resource/Arena.hpp"

#include "Engine/Core/Math/Color.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Resource/Mesh.hpp"

blk::Result
blk::create_arena(const Context& context, Arena& arena)
{
	arena = {};

	// A failure leaves everything created so far without an owner, so we tear the whole `arena` down before returning.

	if (const Result result = create_host_device_buffer(
			context,
			sizeof(Vertex_UV) * MAX_VERTEX_COUNT,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			arena.vertex_buffer
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create arena vertex buffer\n");
		destroy_arena(context, arena);

		return result;
	}

	if (const Result result = create_host_device_buffer(
			context,
			sizeof(Index) * MAX_INDEX_COUNT,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			arena.index_buffer
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create arena index buffer\n");
		destroy_arena(context, arena);

		return result;
	}

	if (const Result result = create_buffer(
			context,
			sizeof(Color_RGBA<uint8_t>) * TEXTURE_WIDTH * TEXTURE_HEIGHT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			arena.texture_buffer
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create arena texture staging buffer\n");
		destroy_arena(context, arena);

		return result;
	}

	// Create the host handle to device resource maps. These capacities are only a starting point, the maps grow on
	// demand. Only `materials` has a hard limit, because `Descriptor_Layouts::pool` is sized for `MAX_MATERIAL_COUNT`
	// descriptor sets.

	if (const Result result = create_hash_map(arena.meshes, context.allocator, MAX_MATERIAL_COUNT, 0.75f);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create arena meshes hash map\n");
		destroy_arena(context, arena);

		return result;
	}

	if (const Result result = create_hash_map(arena.textures, context.allocator, MAX_MATERIAL_COUNT * 3, 0.75f);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create arena textures hash map\n");
		destroy_arena(context, arena);

		return result;
	}

	if (const Result result = create_hash_map(arena.materials, context.allocator, MAX_MATERIAL_COUNT, 0.75f);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create arena materials hash map\n");
		destroy_arena(context, arena);

		return result;
	}

	return Result::SUCCESS;
}

void
blk::destroy_arena(const Context& context, Arena& arena)
{
}
