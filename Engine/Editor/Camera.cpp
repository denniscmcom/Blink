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
/// Pitch is clamped just short of 90 degrees. At exactly straight up or down the forward vector becomes parallel to
/// `world_up`, so their cross product collapses to zero and the right vector is undefined.
constexpr float PITCH_LIMIT = 1.55f;
}  // namespace

void
blk::update_editor_camera(
	World& world,
	const Pool_Handle<Camera>& handle,
	const Input_State& input_state,
	double delta_time
)
{
	// Get camera node to update the transform component.
	Node* node = get_node(world, handle);

	if (!node)
	{
		BLK_ERROR("Failed to find editor camera node\n");

		return;
	}

	// TODO (Knowledge): This is copy pasted from the internet and I do not currently understand all of it.

	// Mouse right increases yaw, which rotates `+Z` towards `+X`. Mouse down increases pitch, which tilts `+Z` down.
	node->transform.rotation.y += static_cast<float>(input_state.mouse_delta_x) * MOUSE_SENSITIVITY;
	node->transform.rotation.x += static_cast<float>(input_state.mouse_delta_y) * MOUSE_SENSITIVITY;

	if (node->transform.rotation.x > PITCH_LIMIT)
	{
		node->transform.rotation.x = PITCH_LIMIT;
	}
	else if (node->transform.rotation.x < -PITCH_LIMIT)
	{
		node->transform.rotation.x = -PITCH_LIMIT;
	}

	// Initialize the rotation matrix.
	const Matrix4 rotation_matrix = init_rotation_matrix(node->transform.rotation);

	// Engine's forward convention `+Z`. We use a `Vector4` because we have to multiply it by the `rotation_matrix`.
	constexpr Vector4 world_forward = {
		.x = 0.0f,
		.y = 0.0f,
		.z = 1.0f,
	};

	// Compute the forward vector.
	const Vector4 forward4 = rotation_matrix * world_forward;

	// Since we only need a 3D forward vector, we can remove the `w` component.
	const Vector3 forward = {
		.x = forward4.x,
		.y = forward4.y,
		.z = forward4.z,
	};

	// Engine's up convention `+Y`.
	constexpr Vector3 world_up = {
		.x = 0.0f,
		.y = 1.0f,
		.z = 0.0f,
	};

	// With the up and forward vector we can compute the right vector.
	const Vector3 right = compute_unit_vector(compute_cross_product(world_up, forward));

	// The camera velocity to each direction. We'll compute it later based on input.
	Vector3 velocity = {
		.x = 0.0f,
		.y = 0.0f,
		.z = 0.0f,
	};

	// Press `W` to move forward.
	if (is_key_held(input_state, Key::KEYBOARD_W))
	{
		velocity += forward;
	}

	// Press `S` to move backwards.
	if (is_key_held(input_state, Key::KEYBOARD_S))
	{
		velocity += -forward;
	}

	// Press `D` to move to the right.
	if (is_key_held(input_state, Key::KEYBOARD_D))
	{
		velocity += right;
	}

	// Press `A` to move to the left.
	if (is_key_held(input_state, Key::KEYBOARD_A))
	{
		velocity += -right;
	}

	// Now we right foot up, left foot slide...

	// Press `Q` to go down.
	if (is_key_held(input_state, Key::KEYBOARD_Q))
	{
		velocity += -world_up;
	}

	// Press `E` to go up.
	if (is_key_held(input_state, Key::KEYBOARD_E))
	{
		velocity += world_up;
	}

	// TODO (Knowledge): What is that for?
	if (compute_vector_magnitude_squared(velocity) > 0.0f)
	{
		// TODO (Knowledge): Explain it.
		velocity = compute_unit_vector(velocity);
		node->transform.position += MOVE_SPEED * static_cast<float>(delta_time) * velocity;
	}
}
