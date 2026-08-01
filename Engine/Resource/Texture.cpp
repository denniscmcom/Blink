// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Texture.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

#include "stb_image.h"

#include <math.h>
#include <string>

namespace
{
blk::Resource_Storage<blk::Texture> texture_storage = {};
}  // namespace

blk::Pool_Handle<blk::Texture>
blk::load_texture(const char* stem)
{
	std::string path = "./Assets/Textures/";
	path += stem;
	path += ".png";

	if (is_resource_loaded(texture_storage, path.c_str()))
	{
		return get_resource_handle(texture_storage, path.c_str());
	}

	int width = 0;
	int height = 0;
	int channel_count = 0;

	stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channel_count, STBI_rgb_alpha);

	if (!pixels)
	{
		BLK_FATAL("Failed to load texture\n");
	}

	const auto* begin = reinterpret_cast<const Color_RGBA<uint8_t>*>(pixels);

	Texture texture = {};
	texture.pixels.assign(begin, begin + static_cast<size_t>(width) * static_cast<size_t>(height));
	texture.width = static_cast<uint32_t>(width);
	texture.height = static_cast<uint32_t>(height);

	stbi_image_free(pixels);

	return store_resource(texture_storage, texture, stem, path.c_str());
}

void
blk::unload_texture(const Pool_Handle<Texture> handle)
{
	release_resource(texture_storage, handle);
}

blk::Texture*
blk::get_texture(const Pool_Handle<Texture> handle)
{
	return get_resource(texture_storage, handle);
}

blk::Color_RGB<float>
blk::convert_srgb_to_linear(const Color_RGB<float>& color)
{
	return {
		.r = powf(color.r, 2.2f),
		.g = powf(color.g, 2.2f),
		.b = powf(color.b, 2.2f),
	};
}
