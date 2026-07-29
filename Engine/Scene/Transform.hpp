#pragma once

#include "Core/Math/Vector.hpp"

namespace blk
{
struct Transform
{
	Vector3 position;
	// TODO: Use quaternions.
	Vector3 rotation;
	Vector3 scale = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
};

Matrix4 make_transform_matrix(const Transform& transform);
}  // namespace blk
