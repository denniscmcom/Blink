// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Texture.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Resource.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

#include "stb_image.h"

#include <format>
#include <math.h>
#include <string>

namespace
{
blk::Resource_Storage<blk::Texture> texture_storage = {};
}  // namespace

blk::Pool_Handle<blk::Texture>
blk::load_texture(const char* stem)
{
	const uint64_t hash = get_resource_hash(stem, Resource_Type::TEXTURE);

	return load_texture(hash);
}

blk::Pool_Handle<blk::Texture>
blk::load_texture(uint64_t hash)
{
	const std::string path = std::format("Assets/{}.btexture", hash);

	if (is_resource_loaded(texture_storage, hash))
	{
		return get_resource_handle(texture_storage, hash);
	}

	File* file = open_file(path.c_str(), File_Access_Mode::READ);

	if (!file)
	{
		BLK_FATAL("Failed to open asset: %s\n", path.c_str());
	}

	const uint64_t file_size = get_file_size(file);
	auto buffer = static_cast<char*>(malloc(file_size));
	read_file(file, buffer, file_size);
	close_file(file);

	Serial src_serial(buffer, file_size);

	if (src_serial.read<Magic>() != BLINK_MAGIC)
	{
		BLK_FATAL("Source buffer is not Blink format\n");
	}

	if (src_serial.read<Magic>() != TEXTURE_MAGIC)
	{
		BLK_FATAL("Source buffer is not a Blink texture\n");
	}

	if (src_serial.read<uint8_t>() != TEXTURE_VERSION)
	{
		BLK_FATAL("Expected Blink texture version %u\n", TEXTURE_VERSION);
	}

	Texture texture = {};

	texture.width = src_serial.read<uint32_t>();
	texture.height = src_serial.read<uint32_t>();
	texture.pixels.reserve(texture.width * texture.height);

	for (uint64_t i = 0; i < texture.width * texture.height; i++)
	{
		Color_RGBA<uint8_t> pixel = {};

		pixel.r = src_serial.read<uint8_t>();
		pixel.g = src_serial.read<uint8_t>();
		pixel.b = src_serial.read<uint8_t>();
		pixel.a = src_serial.read<uint8_t>();

		texture.pixels.push_back(pixel);
	}

	return store_resource(texture_storage, texture, hash, "");
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
