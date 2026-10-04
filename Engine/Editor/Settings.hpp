// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;

/// Draws the settings widget.
/// It displays any type of settings (node, world, etc). It is located on the right of the screen.
void draw_settings(Editor_Context& context);
}  // namespace blk
