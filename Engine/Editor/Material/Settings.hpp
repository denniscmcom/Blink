// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;

/// Draws the material settings widget at the right side of the screen in `Editor_Mode::MATERIAL`.
/// This widget allows material creation and updating an existing material.
void draw_material_settings(Editor_Context& context);
}  // namespace blk
