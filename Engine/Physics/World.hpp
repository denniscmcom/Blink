// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <box3d/box3d.h>

namespace blk
{
struct Physics
{
	b3WorldId world_id;
};

/// Creates the physics world.
Physics create_physics_world();
/// Destroys the physics world.
void destroy_physics_world(Physics& physics);

/// Draws the physics world's debug view.
void draw_physics_debug(const Physics& physics);
}  // namespace blk
