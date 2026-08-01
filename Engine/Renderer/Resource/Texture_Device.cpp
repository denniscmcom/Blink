// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Resource/Texture_Device.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Resource/Arena.hpp"
#include "Engine/Resource/Texture.hpp"

blk::Texture_Device
blk::transfer_texture(const Context& context, Arena& arena, const Pool_Handle<Texture>& handle, const VkFormat format)
{
	const Texture* texture = get_texture(handle);

	if (!texture)
	{
		BLK_FATAL("Failed to get texture\n");
	}

	const VkDeviceSize texture_size = sizeof(Color_RGBA<uint8_t>) * texture->pixels.size();

	if (arena.texture_buffer.size < texture_size)
	{
		BLK_FATAL("Texture staging buffer is too small\n");
	}

	if (!arena.texture_buffer.map)
	{
		map_buffer(context, arena.texture_buffer);
	}

	update_buffer(arena.texture_buffer, texture->pixels.data(), texture_size, 0);
	unmap_buffer(context, arena.texture_buffer);

	Texture_Device texture_device = {};
	texture_device.handle = handle;
	texture_device.image = create_image(
		context,
		texture->width,
		texture->height,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	// FIXME: I think here should be a transition_image_layout call. Verify with vulkan tutorial.

	copy_buffer_to_image(context, arena.texture_buffer, texture_device.image.image, texture->width, texture->height);
	arena.textures.insert({handle, texture_device});

	return texture_device;
}
