// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Editor_Context;
struct Input_State;

void create_editor_camera(Editor_Context& context);
void destroy_editor_camera(Editor_Context& context);
void update_editor_camera(const Editor_Context& context, double delta_time, const Input_State& input_state);
void activate_editor_camera(Editor_Context& context);
}  // namespace blk
