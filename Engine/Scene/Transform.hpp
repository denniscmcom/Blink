// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Vector.hpp"

namespace blk
{
struct Matrix4;

/// Spatial transform data of a `Node`.
struct Transform
{
	/// Position data.
	Vector3 position;
	// TODO: Use quaternions.
	/// Rotation data.
	Vector3 rotation;
	/// Scale data.
	Vector3 scale = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
};

// TODO (Bug): `Node` transforms are not inherited. This composes a single `Transform` with no knowledge of the tree,
// and `Renderer.cpp` feeds its result straight to the draw command, so moving a parent node does not move its
// children. `Scene_Graph` should own a `world_transform` per `Node` and a pass that walks the tree root-down filling
// it -`world = parent_world * local`-, and `Renderer/` should read that instead of composing per node.

/// Initializes the transform matrix given a `Transform`.
Matrix4 init_transform_matrix(const Transform& transform);
}  // namespace blk
