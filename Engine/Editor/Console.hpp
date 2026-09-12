// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;

/// Logs a message to the editor console.
void log_editor(const char* msg);
/// Draws the editor console.
///
/// The console is at the bottom of the screen, above the status bar, and its width is based on the sides widgets to the
/// left and right.
void draw_console(Editor_Context& context);
}  // namespace blk
