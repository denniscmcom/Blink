// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Context.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Editor/Camera.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/World.hpp"

void
blk::activate_world_mode_editor_camera(Editor_World_Context& context)
{
	BLK_CHECK(context.game_world);

	if (context.game_world->active_camera_handle != context.editor_camera_handle)
	{
		context.game_camera_handle = context.game_world->active_camera_handle;
		context.game_world->active_camera_handle = context.editor_camera_handle;
		context.is_editor_camera_active = true;
	}
}

blk::Result
blk::create_editor_context(
	Allocator* allocator,
	const Input_State* input_state,
	World* game_world,
	Editor_Context& context
)
{
	if (!BLK_VERIFY(allocator) || !BLK_VERIFY(input_state) || !BLK_VERIFY(game_world))
	{
		return Result::INVALID_ARGUMENTS;
	}

	context = {};
	context.input_state = input_state;

	// Create `context.world_context`.

	Editor_World_Context world_context = {};
	world_context.game_world = game_world;

	// Create the editor camera for `Editor_Mode::WORLD`.
	world_context.editor_camera_handle = spawn_camera(*game_world, "Editor camera", game_world->scene_graph.root);

	if (!BLK_VERIFY(world_context.editor_camera_handle != POOL_HANDLE_NONE<Camera>))
	{
		destroy_editor_context(context);

		return Result::INVALID_ARGUMENTS;
	}

	// Activate editor camera for `Editor_Mode::WORLD` by default.
	activate_world_mode_editor_camera(world_context);

	// Do not forget to assign world context to the general context.
	context.world_context = world_context;

	// Create `context.material_context`.

	Editor_Material_Context material_context = {};

	// First, we create the world for `Editor_Mode::MATERIAL`.
	if (const Result result = create_world(allocator, material_context.world); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create the world for the material editor mode\n");
		destroy_editor_context(context);

		return result;
	}

	// Create the editor camera for `Editor_Mode::MATERIAL`.
	material_context.camera_handle =
		spawn_camera(material_context.world, "Editor camera", material_context.world.scene_graph.root);

	if (!BLK_VERIFY(material_context.camera_handle != POOL_HANDLE_NONE<Camera>))
	{
		destroy_editor_context(context);

		return Result::INVALID_ARGUMENTS;
	}

	// Activate the previously created camera.
	material_context.world.active_camera_handle = material_context.camera_handle;

	// Now, we spawn a simple sphere to preview materials. This also could be a model loaded my the user.
	material_context.material_prop_handle =
		spawn_prop(material_context.world, "Material_Sphere", material_context.world.scene_graph.root);

	if (!BLK_VERIFY(material_context.material_prop_handle != POOL_HANDLE_NONE<Prop>))
	{
		destroy_editor_context(context);

		return Result::INVALID_ARGUMENTS;
	}

	// Attach a UV sphere to the material prop node.
	const Pool_Handle<Mesh> material_sphere_mesh = compute_uv_sphere(1.0f, 18, 32);

	if (const Result result =
			attach_mesh(material_context.world, material_context.material_prop_handle, material_sphere_mesh);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to attach the preview sphere mesh to the material prop\n");
		destroy_editor_context(context);

		return result;
	}

	// Assign the material context to the general context.
	context.material_context = material_context;

	return Result::SUCCESS;
}

void
blk::destroy_editor_context(Editor_Context& context)
{
	// The editor camera lives in the game world, which the editor does not own. `despawn` clears
	// `active_camera_handle` when it is the camera being despawned, so if the editor camera is the active one we hand
	// the game camera back before despawning ours.
	if (context.world_context.game_world)
	{
		if (context.world_context.game_world->active_camera_handle == context.world_context.editor_camera_handle)
		{
			context.world_context.game_world->active_camera_handle = context.world_context.game_camera_handle;
		}

		despawn(*context.world_context.game_world, context.world_context.editor_camera_handle);
	}

	destroy_world(context.material_context.world);

	context = {};
}
