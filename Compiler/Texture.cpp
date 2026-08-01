// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Texture.hpp"

#include "Compiler/Shared.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/Log.hpp"

#include <stb_image.h>

#include <array>

namespace
{
using Png_Magic = std::array<uint8_t, 8>;

constexpr char TEXTURE_MAGIC[4] = {'T', 'E', 'X', 'T'};
constexpr Png_Magic PNG_MAGIC = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
constexpr uint8_t VERSION = 1;
}  // namespace

void
blk::compile_texture(Serial& src_serial, Serial& dst_serial)
{
	if (src_serial.read<Png_Magic>() != PNG_MAGIC)
	{
		BLK_FATAL("Source buffer is not a PNG\n");
	}

	int width = 0;
	int height = 0;
	int channel_count = 0;

	stbi_uc* pixels = stbi_load_from_memory(
		reinterpret_cast<const stbi_uc*>(src_serial.buffer()),
		static_cast<int>(src_serial.size()),
		&width,
		&height,
		&channel_count,
		STBI_rgb_alpha
	);

	if (!pixels)
	{
		BLK_FATAL("Failed to load texture\n");
	}

	dst_serial.write(BLINK_MAGIC, 4);
	dst_serial.write(TEXTURE_MAGIC, 4);

	dst_serial.write(VERSION);

	dst_serial.write(static_cast<uint32_t>(width));
	dst_serial.write(static_cast<uint32_t>(height));

	const size_t bytes_per_pixel = channel_count * sizeof(stbi_uc);
	dst_serial.write(pixels, static_cast<size_t>(width) * static_cast<size_t>(height) * bytes_per_pixel);
}
