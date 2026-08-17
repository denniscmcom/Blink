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

Pool_Handle<Camera> create_editor_camera(World& world);
void destroy_editor_camera(World& world, Pool_Handle<Camera>& handle);
void update_editor_camera(
	const World& world,
	const Pool_Handle<Camera>& handle,
	const Input_State& input_state,
	double delta_time
);
}  // namespace blk
