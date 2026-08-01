// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Vector.hpp"

namespace blk
{
struct Matrix4;

struct Transform
{
	Vector3 position;
	// TODO: Use quaternions.
	Vector3 rotation;
	Vector3 scale = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
};

Matrix4 make_transform_matrix(const Transform& transform);
}  // namespace blk
