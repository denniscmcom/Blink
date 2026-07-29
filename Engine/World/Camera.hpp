#pragma once

#include "Core/Math/Unit.hpp"
#include "Scene/Node.hpp"
#include "Scene/Scene_Graph.hpp"

namespace blk
{
struct Vector3;
struct Matrix4;

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
