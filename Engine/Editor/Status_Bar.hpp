// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;

/// Draws the status bar.
///
/// The status bar is at the bottom of the screen and shows various information depending on `Editor_Mode`.
void draw_status_bar(Editor_Context& context);
}  // namespace blk
