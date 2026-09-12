// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/World/Camera.hpp"

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Unit.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Scene/Graph.hpp"
#include "Engine/Scene/Node.hpp"

blk::Matrix4
blk::init_view_matrix(Scene_Graph& scene_graph, const Camera& camera)
{
	const Node* node = get(scene_graph.nodes, camera.node_handle);

	if (!BLK_VERIFY(node))
	{
		// This should not happen except when `scene_graph` is corrupted.
		return init_identity_matrix();
	}

	const Matrix4 rotation_matrix = init_rotation_matrix(node->transform.rotation);

	const Vector4 forward4 = rotation_matrix * Vector4{.x = 0.0f, .y = 0.0f, .z = 1.0f, .w = 0.0f};
	const Vector4 up4 = rotation_matrix * Vector4{.x = 0.0f, .y = 1.0f, .z = 0.0f, .w = 0.0f};

	const Vector3 eye = node->transform.position;
	const Vector3 forward = {.x = forward4.x, .y = forward4.y, .z = forward4.z};
	const Vector3 up = {.x = up4.x, .y = up4.y, .z = up4.z};

	return init_look_at_matrix(eye, eye + forward, up);
}

blk::Matrix4
blk::init_projection_matrix(const Camera& camera, const float aspect_ratio)
{
	const Radians fov_radians = to_radians(camera.fov);

	return init_perspective_matrix(fov_radians, aspect_ratio, camera.near_plane, camera.far_plane);
}
