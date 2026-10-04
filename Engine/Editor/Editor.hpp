// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

namespace blk
{
struct Allocator;
struct World;
struct Input_State;
struct Editor_Context;
enum class Result;

/// Creates the editor.
Result create_editor(World* game_world, Allocator* allocator);
/// Destroys the editor.
void destroy_editor();
/// Updates the editor and returns its current context.
/// Called per-frame.
Editor_Context update_editor(const Input_State& input_state, double delta_time);
/// Checks if the game simulation has been paused from the editor.
bool is_game_simulation_paused();
/// Gets the current active world.
World* get_active_world();
}  // namespace blk
