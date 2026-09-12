// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#define STB_IMAGE_IMPLEMENTATION

#include "Compiler/Texture.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Storage.hpp"
#include "Engine/Resource/Texture.hpp"

#include <stb_image.h>

namespace
{
/// PNG magic number.
constexpr uint8_t PNG_MAGIC[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
}  // namespace

// TODO (Bug): we are not handling the resizing of `output_serial` if needed.
// How to do it without much repetition?
void
blk::compiler::compile_texture(Serial& input_serial, Serial& output_serial)
{
	// `output_serial.buffer` should be pre-allocated by the caller.
	if (!BLK_VERIFY(output_serial.buffer))
	{
		return;
	}

	if (!input_serial.buffer)
	{
		// We do not have a texture in buffer to compile.
		return;
	}

	// Verify PNG magic number.

	uint8_t png_magic[8] = {};

	BLK_IF_NOT_SUCCESS(read(input_serial, png_magic))
	{
		BLK_FATAL("Failed to read PNG magic number\n");
	}

	if (memcmp(png_magic, PNG_MAGIC, 8) != 0)
	{
		BLK_FATAL("Invalid PNG magic number; file is corrupted\n");
	}

	// Load PNG data from file.

	int width = 0;
	int height = 0;
	int channel_count = 0;

	// `stb_image` parses the PNG header itself, so it needs the whole file – including the magic number we just read
	// past. Handing it `input_serial.position` instead would give it a file with no header, which it rejects.
	stbi_uc* pixels = stbi_load_from_memory(
		reinterpret_cast<const stbi_uc*>(input_serial.buffer),
		static_cast<int>(input_serial.size),
		&width,
		&height,
		&channel_count,
		STBI_rgb_alpha
	);

	if (!pixels)
	{
		BLK_FATAL("Failed to load PNG file\n");
	}

	// The follow writes must be in sync with the format `Formats/BTEXTURE.bt`.

	// Write Blink magic number.

	BLK_IF_NOT_SUCCESS(write(output_serial, BLINK_MAGIC))
	{
		BLK_FATAL("Failed to write Blink magic number\n");
	}

	// Write Blink texture magic number.

	BLK_IF_NOT_SUCCESS(write(output_serial, TEXTURE_MAGIC))
	{
		BLK_FATAL("Failed to write Blink texture magic number\n");
	}

	// Write texture version.

	BLK_IF_NOT_SUCCESS(write(output_serial, TEXTURE_VERSION))
	{
		BLK_FATAL("Failed to write texture version\n");
	}

	// Write texture size.

	BLK_IF_NOT_SUCCESS(write(output_serial, static_cast<uint32_t>(width)))
	{
		BLK_FATAL("Failed to write texture width\n");
	}

	BLK_IF_NOT_SUCCESS(write(output_serial, static_cast<uint32_t>(height)))
	{
		BLK_FATAL("Failed to write texture height\n");
	}

	// Write the texture data.

	// `channel_count` reports the channels of the source file, not the decoded ones. `STBI_rgb_alpha` always
	// decodes to 4 channels regardless of the source.
	constexpr size_t bytes_per_pixel = STBI_rgb_alpha * sizeof(stbi_uc);

	BLK_IF_NOT_SUCCESS(
		write(output_serial, pixels, static_cast<size_t>(width) * static_cast<size_t>(height) * bytes_per_pixel)
	)
	{
		BLK_FATAL("Failed to write texture data\n");
	}

	// We can free the pixels memory now.
	stbi_image_free(pixels);
}
