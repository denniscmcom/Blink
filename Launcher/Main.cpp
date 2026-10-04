// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Console.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Editor.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Debug.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Platform/Time.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/Material.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Resource/Shader.hpp"
#include "Engine/Resource/Texture.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/Camera.hpp"
#include "Engine/World/Stream.hpp"
#include "Engine/World/World.hpp"
#include "Game/Game.hpp"

#include <Windows.h>

/// The entry point of the engine.
///
/// It owns the startup order, the frame loop and the teardown order. Nothing below it decides when anything is created
/// or destroyed.
///
/// Anything that fails during startup is fatal – there is no engine to fall back to – so we crash there instead of
/// unwinding what was already created.
BLK_ENTRY()
{
	// ============================================================================
	// Log sinks.
	// ============================================================================

	// These come first so that everything after this point can report failures. `log_editor` buffers into the console
	// widget, which does not need the editor to exist yet.
	BLK_IF_NOT_SUCCESS(blk::create_log_sink(blk::log_debugger))
	{
		// We have no sink to report this through, so we just fail.
		return 1;
	}

	BLK_IF_NOT_SUCCESS(blk::create_log_sink(blk::log_editor))
	{
		BLK_FATAL("Failed to create the editor log sink\n");
	}

	// ============================================================================
	// Allocator.
	// ============================================================================

	// The top-level allocator. It backs everything the launcher creates – the application, the window, the frame timer,
	// the resource storages, the world and the editor context – and lives for the whole run.
	//
	// `Renderer/` is the exception: it owns its own allocator because its lifetime is tied to the device rather than to
	// the process.
	constexpr size_t LAUNCHER_ALLOCATOR_CAPACITY = 1'024 * 1'024 * 1'024;

	blk::Allocator allocator = {};

	BLK_IF_NOT_SUCCESS(blk::create_arena_allocator(allocator, LAUNCHER_ALLOCATOR_CAPACITY))
	{
		BLK_FATAL("Failed to create the launcher allocator\n");
	}

	// ============================================================================
	// Platform.
	// ============================================================================

	BLK_IF_NOT_SUCCESS(blk::create_application(&allocator, hInstance))
	{
		BLK_FATAL("Failed to create the application\n");
	}

	BLK_IF_NOT_SUCCESS(blk::create_window(&allocator, BLK_PROJECT_NAME))
	{
		BLK_FATAL("Failed to create the window\n");
	}

	size_t client_size_x = 0;
	size_t client_size_y = 0;

	BLK_IF_NOT_SUCCESS(blk::get_window_client_size(client_size_x, client_size_y))
	{
		BLK_FATAL("Failed to get the window client rectangle\n");
	}

	blk::Timer* frame_timer = nullptr;

	BLK_IF_NOT_SUCCESS(blk::create_timer(&allocator, frame_timer))
	{
		BLK_FATAL("Failed to create the frame timer\n");
	}

	// ============================================================================
	// Resource storages.
	// ============================================================================

	// Each storage is a global in its own translation unit, so they have to be created explicitly before anything calls
	// `load_mesh`, `load_shader` and friends. That includes `create_renderer`, which loads its shader modules while it
	// starts up, so the storages come before it and not after.

	BLK_IF_NOT_SUCCESS(blk::create_shader_storage(&allocator))
	{
		BLK_FATAL("Failed to create the shader storage\n");
	}

	BLK_IF_NOT_SUCCESS(blk::create_texture_storage(&allocator))
	{
		BLK_FATAL("Failed to create the texture storage\n");
	}

	BLK_IF_NOT_SUCCESS(blk::create_material_storage(&allocator))
	{
		BLK_FATAL("Failed to create the material storage\n");
	}

	BLK_IF_NOT_SUCCESS(blk::create_mesh_storage(&allocator))
	{
		BLK_FATAL("Failed to create the mesh storage\n");
	}

	// ============================================================================
	// Renderer.
	// ============================================================================

	BLK_IF_NOT_SUCCESS(blk::create_renderer(client_size_x, client_size_y))
	{
		BLK_FATAL("Failed to create the renderer\n");
	}

	// ============================================================================
	// Game.
	// ============================================================================

	blk::game::Game_Context game_context = {};

	// `Game_Context` owns the world but `create_game` has no allocator to build it with, so we create it here and hand
	// it over already usable – with its pools, its hash map and its root node.
	BLK_IF_NOT_SUCCESS(blk::create_world(&allocator, {}, game_context.world))
	{
		BLK_FATAL("Failed to create the game world\n");
	}

	game_context.world.settings.stream.generate = blk::game::spawn_chunk;

	BLK_IF_NOT_SUCCESS(blk::game::create_game(game_context))
	{
		BLK_FATAL("Failed to create the game\n");
	}

	// The game fills the world settings the renderer binds once, so baking comes after it.
	BLK_IF_NOT_SUCCESS(blk::bake_renderer(game_context.world.settings))
	{
		BLK_FATAL("Failed to bake the renderer\n");
	}

	// ============================================================================
	// Editor.
	// ============================================================================

	blk::Input_State input_state = {};

	BLK_IF_NOT_SUCCESS(blk::create_editor(&game_context.world, &allocator))
	{
		BLK_FATAL("Failed to create the editor\n");
	}

	// ============================================================================
	// Frame loop.
	// ============================================================================

	// `get_elapsed_seconds` reports the time since the timer was created, so the first delta is
	// measured from here and not from the start of the process.
	double previous_frame_seconds = 0.0;
	blk::get_elapsed_seconds(frame_timer, previous_frame_seconds);

	while (blk::is_application_running())
	{
		double current_frame_seconds = 0.0;
		blk::get_elapsed_seconds(frame_timer, current_frame_seconds);

		const double delta_time = current_frame_seconds - previous_frame_seconds;
		previous_frame_seconds = current_frame_seconds;

		blk::update_input(input_state);
		const blk::Editor_Context editor_context = blk::update_editor(input_state, delta_time);

		if (!blk::is_game_simulation_paused())
		{
			blk::game::update_game(game_context, delta_time, input_state);
		}

		blk::World* active_world = blk::get_active_world();

		blk::Vector3 node_offset = {};
		blk::update_world(*active_world, node_offset);

		// Nothing moves nodes after this point, and both the camera view and `update_frame` read their world matrices.
		blk::update_node_transforms(active_world->scene_graph);

		// The window can be resized, so we sample the client rectangle every frame instead of reusing the one the
		// renderer was created with.
		BLK_IF_NOT_SUCCESS(blk::get_window_client_size(client_size_x, client_size_y))
		{
			// Without it we cannot build a projection matrix, so we skip the frame.
			continue;
		}

		if (client_size_x == 0 || client_size_y == 0)
		{
			// The window is minimized or has no area. There is nothing to render, and the projection would divide by
			// zero.
			continue;
		}

		blk::Renderer_Settings renderer_settings = {};
		renderer_settings.is_debug_triangle_enabled = editor_context.is_showing_triangles;

		if (const blk::Camera* camera = blk::get_camera(*active_world, active_world->active_camera_handle))
		{
			const auto aspect_ratio = static_cast<float>(client_size_x) / static_cast<float>(client_size_y);

			renderer_settings.view = blk::init_view_matrix(active_world->scene_graph, *camera);
			renderer_settings.projection = blk::init_projection_matrix(*camera, aspect_ratio);

			if (const blk::Node* camera_node = blk::get_node(active_world->scene_graph, camera->node_handle))
			{
				const blk::Vector4& camera_position = camera_node->world_matrix.columns[3];

				renderer_settings
					.view_position = {.x = camera_position.x, .y = camera_position.y, .z = camera_position.z};
			}
		}

		blk::update_frame(active_world->scene_graph, renderer_settings, active_world->settings);
		blk::render_frame(active_world->settings);
	}

	// ============================================================================
	// Teardown.
	// ============================================================================

	blk::destroy_editor();

	blk::game::destroy_game(game_context);
	blk::destroy_world(game_context.world);

	blk::destroy_renderer();

	blk::destroy_mesh_storage();
	blk::destroy_material_storage();
	blk::destroy_texture_storage();
	blk::destroy_shader_storage();

	blk::destroy_timer(frame_timer);
	blk::destroy_window();
	blk::destroy_application();

	blk::destroy_arena_allocator(allocator);

	return 0;
}
