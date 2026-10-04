// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;

/// Draws the scene graph.
///
/// The scene graph widget displays the hierarchical representation of the world. It is located on the left of the
/// screen.
void draw_scene_graph(Editor_Context& context);
}  // namespace blk
