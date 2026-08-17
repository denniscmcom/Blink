// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Context.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Editor/Camera.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/World.hpp"

blk::World_Context::World_Context(World* game_world) : _game_world(game_world)
{
	editor_camera_handle = create_editor_camera(*_game_world);

	// Activate editor camera for `Editor_Mode::WORLD` by default.
	activate_world_mode_editor_camera(*this);
}

void
blk::activate_world_mode_editor_camera(World_Context& context)
{
	BLK_CHECK(context._game_world);

	if (context._game_world->active_camera_handle != context.editor_camera_handle)
	{
		context.game_camera_handle = context._game_world->active_camera_handle;
		context._game_world->active_camera_handle = context.editor_camera_handle;
		context.is_editor_camera_active = true;
	}
}

blk::Material_Context::Material_Context()
{
	camera_handle = create_editor_camera(world);
	world.active_camera_handle = camera_handle;
	material_prop_handle = spawn_prop(world, "Material_Sphere", world.scene_graph.root);
	Node* material_node = get_entity_node(world, material_prop_handle);

	if (!material_node)
	{
		BLK_ERROR("Failed to find material prop node\n");

		return;
	}

	Mesh_Instance mesh_instance = {};
	mesh_instance.mesh_handle = compute_uv_sphere(1.0f, 18, 32);

	material_node->mesh_instance = mesh_instance;
}

blk::Editor_Context::Editor_Context(const Input_State* input_state, World* game_world)
	: _input_state(input_state), world_context(game_world)
{
}
