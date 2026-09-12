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

/// A camera entity.
struct Camera
{
	/// A handle to its node in `Scene_Graph`.
	Pool_Handle<Node> node_handle;
	/// The field of view in degrees.
	/// Default value is 45.0.
	Degrees fov = {45.0f};
	/// The closest boundary in meters where objects begin to be rendered.
	/// Default value is 0.1.
	float near_plane = 0.1f;
	/// The farthest boundary in meters where objects are still rendered.
	/// Default value is 100.0.
	float far_plane = 100.0f;
};

/// Initializes the view matrix of `camera`.
/// @warning The `camera` node should be in `scene_graph`.
Matrix4 init_view_matrix(Scene_Graph& scene_graph, const Camera& camera);
/// Initializes the projection matrix of `camera`.
Matrix4 init_projection_matrix(const Camera& camera, float aspect_ratio);
}  // namespace blk
