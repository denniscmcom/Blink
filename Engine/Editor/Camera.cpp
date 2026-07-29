#include "Editor/Camera.hpp"

#include "Core/Math/Matrix.hpp"
#include "Core/Math/Vector.hpp"
#include "Input/Input.hpp"
#include "Scene/Node.hpp"
#include "World/Camera.hpp"

#include "Context.hpp"
#include "World/World.hpp"

namespace
{
constexpr float MOUSE_SENSITIVITY = 0.0015f;
constexpr float MOVE_SPEED = 5.0f;
constexpr float PITCH_LIMIT = 1.55f;  // ~89 degrees, keeps forward away from world up.
}  // namespace

void
blk::create_editor_camera(Editor_Context& context)
{
	BLK_CHECK(context.world);
	context.editor_camera_handle = spawn_camera(*context.world, "Editor camera", context.world->scene_graph.root);
}

void
blk::destroy_editor_camera(Editor_Context& context)
{
	despawn_camera(*context.world, context.editor_camera_handle);
	context.editor_camera_handle = {};
}

void
blk::update_editor_camera(const Editor_Context& context, const double delta_time, const Input_State& input_state)
{
	BLK_CHECK(context.world);

	const Camera* camera = context.world->cameras.get(context.editor_camera_handle);
	BLK_CHECK(camera);

	Node* node = context.world->scene_graph.nodes.get(camera->node_handle);
	BLK_CHECK(node);

	// Mouse look: rotation.y is yaw, rotation.x is pitch (Euler radians, same convention
	// as make_view_matrix).
	node->transform.rotation.y += -input_state.mouse_delta_x * MOUSE_SENSITIVITY;
	node->transform.rotation.x += -input_state.mouse_delta_y * MOUSE_SENSITIVITY;

	if (node->transform.rotation.x > PITCH_LIMIT)
	{
		node->transform.rotation.x = PITCH_LIMIT;
	}
	else if (node->transform.rotation.x < -PITCH_LIMIT)
	{
		node->transform.rotation.x = -PITCH_LIMIT;
	}

	// Movement basis derived exactly like make_view_matrix: forward is +Z rotated by the
	// node rotation, right is cross(world_up, forward) (left-handed look-at convention).
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
		node->transform.position += (MOVE_SPEED * static_cast<float>(delta_time)) * velocity;
	}
}

void
blk::activate_editor_camera(Editor_Context& context)
{
	BLK_CHECK(context.world);

	if (context.world->active_camera_handle != context.editor_camera_handle)
	{
		context.game_camera_handle = context.world->active_camera_handle;
		context.world->active_camera_handle = context.editor_camera_handle;
		context.is_editor_camera_active = true;
	}
}
