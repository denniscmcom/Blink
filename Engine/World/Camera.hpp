// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Unit.hpp"
#include "Engine/Core/Pool.hpp"

namespace blk
{
struct Matrix4;
struct Node;
struct Scene_Graph;

struct Camera
{
	Pool_Handle<Node> node_handle;
	Degrees fov = {45.0};
	float near_plane = 0.1f;
	float far_plane = 100.0f;
};

Matrix4 make_view_matrix(const Scene_Graph& scene, const Camera& camera);
Matrix4 make_projection_matrix(const Camera& camera, float aspect_ratio);
}  // namespace blk
