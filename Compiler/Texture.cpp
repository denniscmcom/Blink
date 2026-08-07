// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "Compiler/Texture.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Resource_Storage.hpp"
#include "Engine/Resource/Texture.hpp"

#include <stb_image.h>

#include <array>

namespace
{
using Png_Magic = std::array<uint8_t, 8>;

constexpr Png_Magic PNG_MAGIC = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
}  // namespace

void
blk::compile_texture(Serial& input_serial, Serial& output_serial)
{
	if (input_serial.read<Png_Magic>() != PNG_MAGIC)
	{
		BLK_FATAL("Source buffer is not a PNG\n");
	}

	int width = 0;
	int height = 0;
	int channel_count = 0;

	stbi_uc* pixels = stbi_load_from_memory(
		reinterpret_cast<const stbi_uc*>(input_serial.buffer()),
		static_cast<int>(input_serial.size()),
		&width,
		&height,
		&channel_count,
		STBI_rgb_alpha
	);

	if (!pixels)
	{
		BLK_FATAL("Failed to load texture\n");
	}

	output_serial.write(BLINK_MAGIC);
	output_serial.write(TEXTURE_MAGIC);
	output_serial.write(TEXTURE_VERSION);

	output_serial.write(static_cast<uint32_t>(width));
	output_serial.write(static_cast<uint32_t>(height));

	// `channel_count` reports the channels of the source file, not the decoded ones. `STBI_rgb_alpha` always
	// decodes to 4 channels regardless of the source.
	constexpr size_t bytes_per_pixel = STBI_rgb_alpha * sizeof(stbi_uc);
	output_serial.write(pixels, static_cast<size_t>(width) * static_cast<size_t>(height) * bytes_per_pixel);

	stbi_image_free(pixels);
}
