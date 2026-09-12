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
/// Compiles a `.bmaterial`.
///
/// It just copies the contents of `input_serial` to `output_serial` since the contents of the file do not differ at
/// runtime. For a `.bmaterial` file we only create a new file named after its hashed filename, so that the runtime can
/// load it.
void compile_material(const Serial& input_serial, Serial& output_serial);
}  // namespace blk::compiler
