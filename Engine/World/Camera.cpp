#include "World/Camera.hpp"

#include "Core/Math/Matrix.hpp"
#include "Platform/Assert.hpp"
#include "Scene/Node.hpp"
#include "Scene/Scene_Graph.hpp"

blk::Matrix4
blk::make_view_matrix(const Scene_Graph& scene, const Camera& camera)
{
	const Node* node = scene.nodes.get(camera.node_handle);

	if (!BLK_VERIFY(node))
	{
		return make_identity_matrix();
	}

	BLK_CHECK(node);

	const Matrix4 rotation_matrix = make_rotation_matrix(node->transform.rotation);

	const Vector4 forward4 = rotation_matrix * Vector4{.x = 0.0f, .y = 0.0f, .z = 1.0f, .w = 0.0f};
	const Vector4 up4 = rotation_matrix * Vector4{.x = 0.0f, .y = 1.0f, .z = 0.0f, .w = 0.0f};

	const Vector3 eye = node->transform.position;
	const Vector3 forward = {.x = forward4.x, .y = forward4.y, .z = forward4.z};
	const Vector3 up = {.x = up4.x, .y = up4.y, .z = up4.z};

	return make_look_at_matrix(eye, eye + forward, up);
}

blk::Matrix4
blk::make_projection_matrix(const Camera& camera, const float aspect_ratio)
{
	const Radians fov_radians = to_radians(camera.fov);

	return make_perspective_matrix(fov_radians, aspect_ratio, camera.near_plane, camera.far_plane);
}
