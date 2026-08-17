// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

// FIXME: Currently the launcher and renderer also includes ImGui. Ideally only this module include it.

namespace blk
{
struct World;
struct Input_State;
struct Editor_Context;

void create_editor(Editor_Context& context);
void destroy_editor();
void update_editor(Editor_Context& context, double delta_time);
}  // namespace blk
