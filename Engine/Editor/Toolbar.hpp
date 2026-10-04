// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;

/// Draws the toolbar.
///
/// The toolbar's contents depend on `Editor_Mode`. It contains buttons to switch to other `Editor_Mode` and to modify
/// its properties. It is located below the main menu.
void draw_toolbar(Editor_Context& context);
}  // namespace blk
