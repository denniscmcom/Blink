// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Material.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Resource/Material.hpp"

void
blk::compiler::compile_material(const Serial& input_serial, Serial& output_serial)
{
	BLK_IF_NOT_SUCCESS(write(output_serial, input_serial.buffer, input_serial.size))
	{
		BLK_FATAL("Failed to write material data\n");
	}
}
