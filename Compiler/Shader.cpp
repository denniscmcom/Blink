// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Shader.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Resource/Resource_Storage.hpp"
#include "Engine/Resource/Shader.hpp"

#include <array>
#include <stdint.h>

namespace
{
using Spv_Magic = std::array<uint8_t, 4>;

// TODO: Be aware of endianness. it's different in other systems.
constexpr Spv_Magic SPV_MAGIC = {0x03, 0x02, 0x23, 0x07};
}  // namespace

void
blk::compile_shader(Serial& input_serial, Serial& output_serial)
{
	if (input_serial.read<Spv_Magic>() != SPV_MAGIC)
	{
		BLK_FATAL("Source buffer is not SPV\n");
	}

	output_serial.write(BLINK_MAGIC);
	output_serial.write(SHADER_MAGIC);
	output_serial.write(SHADER_VERSION);

	output_serial.write(input_serial.buffer(), input_serial.size());
}
