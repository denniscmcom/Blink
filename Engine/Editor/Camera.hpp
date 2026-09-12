// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"

namespace blk
{
struct World;
struct Camera;
struct Input_State;

/// Update the editor camera.
///
/// Passes and processes the `input_state` to the camera `handle` when in free fly mode.
void update_editor_camera(
	World& world,
	const Pool_Handle<Camera>& handle,
	const Input_State& input_state,
	double delta_time
);
}  // namespace blk
