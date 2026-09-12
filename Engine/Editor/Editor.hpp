// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;
enum class Result;

/// Creates the editor.
/// @see create_editor_context.
Result create_editor(Editor_Context& context);
/// Destroys the editor.
void destroy_editor();
/// Updates the editor.
///
/// All editor input is handled here, either directly or through `update_editor_camera`, which this function calls. The
/// `draw_*` widget functions only read state, they never handle input nor mutate it.
///
/// Called per-frame by `Launcher/`.
void update_editor(Editor_Context& context, double delta_time);
}  // namespace blk
