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
	/// Rotation data. TODO (Feature): Use quaternions.
	Vector3 rotation;
	/// Scale data.
	Vector3 scale = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
};

/// Initializes the transform matrix given a `Transform`.
Matrix4 init_transform_matrix(const Transform& transform);
}  // namespace blk
