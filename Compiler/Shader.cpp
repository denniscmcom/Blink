// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Shader.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Resource/Shader.hpp"
#include "Engine/Resource/Storage.hpp"

#include <stdint.h>

namespace
{
/// SPV magic number.
///
/// SPIR-V's magic is the word `0x07230203`, which a little-endian module stores as these bytes. A big-endian module
/// would store them reversed, so this check accepts little-endian modules only — which is what every desktop toolchain
/// emits.
constexpr uint8_t SPV_MAGIC[4] = {0x03, 0x02, 0x23, 0x07};
}  // namespace

// TODO (Bug): we are not handling the resizing of `output_serial` if needed.
// How to do it without much repetition?
void
blk::compiler::compile_shader(Serial& input_serial, Serial& output_serial)
{
	// `output_serial.buffer` should be pre-allocated by the caller.
	if (!BLK_VERIFY(output_serial.buffer))
	{
		return;
	}

	if (!input_serial.buffer)
	{
		// We do not have a shader in buffer to compile.
		return;
	}

	// Verify SPV magic number.

	uint8_t spv_magic[sizeof(SPV_MAGIC)] = {};

	BLK_IF_NOT_SUCCESS(read(input_serial, spv_magic))
	{
		BLK_FATAL("Failed to read SPV magic number\n");
	}

	if (memcmp(spv_magic, SPV_MAGIC, sizeof(SPV_MAGIC)) != 0)
	{
		BLK_FATAL("Invalid SPV magic number; file is corrupted\n");
	}

	// The follow writes must be in sync with the format `Formats/BSHADER.bt`.

	// Write Blink magic number.

	BLK_IF_NOT_SUCCESS(write(output_serial, BLINK_MAGIC))
	{
		BLK_FATAL("Failed to write Blink magic number\n");
	}

	// Write Blink shader magic number.

	BLK_IF_NOT_SUCCESS(write(output_serial, SHADER_MAGIC))
	{
		BLK_FATAL("Failed to write Blink shader magic number\n");
	}

	// Write shader version.

	BLK_IF_NOT_SUCCESS(write(output_serial, SHADER_VERSION))
	{
		BLK_FATAL("Failed to write shader version\n");
	}

	// Write shader data.

	// A SPIR-V module starts with the magic number we just read past, and `Resource/Shader.cpp` copies everything after
	// our own header straight into `VkShaderModuleCreateInfo::pCode`, so the module has to be written whole.
	BLK_IF_NOT_SUCCESS(write(output_serial, input_serial.buffer, input_serial.size))
	{
		BLK_FATAL("Failed to write shader data\n");
	}
}
