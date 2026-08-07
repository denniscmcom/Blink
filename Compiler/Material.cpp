// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Material.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Resource/Material.hpp"

void
blk::compile_material(Serial& input_serial, Serial& output_serial)
{
	output_serial.write(input_serial.buffer(), input_serial.size());
}
