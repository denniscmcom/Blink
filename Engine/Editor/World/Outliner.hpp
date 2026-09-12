// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;

/// Draws the outliner.
///
/// The outliner is shown only in `Editor_Mode::WORLD` and is composed of two side widgets, one at the left and the
/// other at the right of the screen. Both are below the menu and above the status bar.
///
/// The widget on the left is the scene graph: it displays the nodes in the scene.
/// The widget on the right is the settings: it displays the settings of the selected node or world settings.
void draw_outliner(Editor_Context& context);
}  // namespace blk
