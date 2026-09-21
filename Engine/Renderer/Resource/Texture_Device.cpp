// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Resource/Texture_Device.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Resource/Arena.hpp"
#include "Engine/Resource/Texture.hpp"

blk::Result
blk::transfer_texture(
	const Context& context,
	Arena& arena,
	Pool_Handle<Texture> host_handle,
	VkFormat format,
	Texture_Device& texture
)
{
	texture = {};

	// Check if `texture` is already in device.
	if (const Texture_Device* texture_ptr = get(arena.textures, host_handle))
	{
		texture = *texture_ptr;

		return Result::SUCCESS;
	}

	// Get texture host data.
	const Texture* texture_host = get_texture(host_handle);

	if (!texture_host)
	{
		BLK_ERROR("Failed to get texture host data to transfer\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Verify if texture size is the same as the texture staging buffer.

	if (!BLK_VERIFY(sizeof(Color_RGBA<uint8_t>) * texture_host->pixels.count == arena.texture_buffer.size))
	{
		return Result::OUT_OF_MEMORY;
	}

	texture.host_handle = host_handle;

	if (!arena.texture_buffer.map)
	{
		// Map the texture buffer if it's not already mapped.
		map_buffer(context, arena.texture_buffer);
	}

	// Copy texture data from host to device texture buffer.
	if (const Result result = update_buffer(
			arena.texture_buffer,
			texture_host->pixels.buffer,
			sizeof(Color_RGBA<uint8_t>) * texture_host->pixels.count,
			0
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to update staging texture buffer\n");
		unload_texture_from_device(context, arena, texture);

		return result;
	}

	unmap_buffer(context, arena.texture_buffer);

	// Create texture image.

	if (const Result result = create_image(
			context,
			texture_host->width,
			texture_host->height,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			texture.image
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create texture image\n");
		unload_texture_from_device(context, arena, texture);

		return result;
	}

	// Copy pixel data from texture staging buffer to image.

	if (const Result result = copy_buffer_to_image(
			context,
			arena.texture_buffer,
			texture.image.image,
			texture_host->width,
			texture_host->height
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to copy pixel data from staging texture buffer to image\n");
		unload_texture_from_device(context, arena, texture);

		return result;
	}

	// Insert host_handle-texture pair to hash map.
	insert(arena.textures, host_handle, texture);

	return Result::SUCCESS;
}

void
blk::unload_texture_from_device(const Context& context, Arena& arena, Texture_Device& texture)
{
}
