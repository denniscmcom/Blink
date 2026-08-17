// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Camera.hpp"

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Event.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/Camera.hpp"
#include "Engine/World/World.hpp"

namespace
{
constexpr float MOUSE_SENSITIVITY = 0.0015f;
constexpr float MOVE_SPEED = 5.0f;
constexpr float PITCH_LIMIT = 1.55f;
}  // namespace

blk::Pool_Handle<blk::Camera>
blk::create_editor_camera(World& world)
{
	return spawn_camera(world, "Editor camera", world.scene_graph.root);
}

void
blk::destroy_editor_camera(World& world, Pool_Handle<Camera>& handle)
{
	despawn_camera(world, handle);
	handle = {};
}

void
blk::update_editor_camera(
	const World& world,
	const Pool_Handle<Camera>& handle,
	const Input_State& input_state,
	double delta_time
)
{
	const Camera* camera = world.cameras.get(handle);

	if (!camera)
	{
		BLK_ERROR("Failed to find editor camera\n");
		return;
	}

	Node* node = world.scene_graph.nodes.get(camera->node_handle);

	if (!node)
	{
		BLK_ERROR("Failed to find editor camera node\n");
		return;
	}

	node->transform.rotation.y += -static_cast<float>(input_state.mouse_delta_x) * MOUSE_SENSITIVITY;
	node->transform.rotation.x += -static_cast<float>(input_state.mouse_delta_y) * MOUSE_SENSITIVITY;

	if (node->transform.rotation.x > PITCH_LIMIT)
	{
		node->transform.rotation.x = PITCH_LIMIT;
	}
	else if (node->transform.rotation.x < -PITCH_LIMIT)
	{
		node->transform.rotation.x = -PITCH_LIMIT;
	}

	const Matrix4 rotation_matrix = make_rotation_matrix(node->transform.rotation);
	const Vector4 forward4 = rotation_matrix * Vector4{.x = 0.0f, .y = 0.0f, .z = 1.0f, .w = 0.0f};

	const Vector3 forward = {.x = forward4.x, .y = forward4.y, .z = forward4.z};
	constexpr Vector3 world_up = {.x = 0.0f, .y = 1.0f, .z = 0.0f};
	const Vector3 right = compute_unit_vector(compute_cross_product(world_up, forward));

	Vector3 velocity = {.x = 0.0f, .y = 0.0f, .z = 0.0f};

	if (is_key_held(input_state, Key::KEYBOARD_W))
	{
		velocity += forward;
	}

	if (is_key_held(input_state, Key::KEYBOARD_S))
	{
		velocity += -forward;
	}

	if (is_key_held(input_state, Key::KEYBOARD_D))
	{
		velocity += right;
	}

	if (is_key_held(input_state, Key::KEYBOARD_A))
	{
		velocity += -right;
	}

	if (is_key_held(input_state, Key::KEYBOARD_Q))
	{
		velocity += -world_up;
	}

	if (is_key_held(input_state, Key::KEYBOARD_E))
	{
		velocity += world_up;
	}

	if (compute_vector_magnitude_squared(velocity) > 0.0f)
	{
		velocity = compute_unit_vector(velocity);
		node->transform.position += MOVE_SPEED * static_cast<float>(delta_time) * velocity;
	}
}
