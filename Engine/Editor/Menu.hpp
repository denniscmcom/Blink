// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;

/// Draws the main menu.
///
/// The main menu is the source of truth of the editor. Every widget is drawn, shown, or hidden by it.
/// This menu is at the top of the screen; similar to a top bar menu in any Windows application.
void draw_menu(Editor_Context& context);
}  // namespace blk
