// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Serial;
}  // namespace blk

namespace blk::compiler
{
/// Compiles a `.png` file into `.btexture`.
///
/// @param output_serial Its buffer should be pre-allocated by the caller and large enough to fit the result.
void compile_texture(Serial& input_serial, Serial& output_serial);
}  // namespace blk::compiler
