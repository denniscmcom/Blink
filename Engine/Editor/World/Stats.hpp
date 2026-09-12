// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;

/// Draws the stats widget.
///
/// This widget is located at the bottom left of the screen, below the outliner and above the status bar in
/// `Editor_Mode::WORLD`. It shows various graphs and performance data about the engine.
void draw_stats(Editor_Context& context);
/// Draws the FPS of the engine.
void draw_fps();
/// Draws the miliseconds per frame.
void draw_ms();
/// Computes performance stats.
///
/// Computation is decoupled from drawing because `draw_stats` and `draw_status_bar` both display the same numbers, and
/// the status bar is drawn first. Computing once per frame keeps them in step and avoids walking the node pool twice.
void compute_stats(const Editor_Context& context, double delta_time);
}  // namespace blk
