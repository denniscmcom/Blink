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
#include "Engine/Platform/Types.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/Material.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Resource/Shader.hpp"
#include "Engine/Resource/Texture.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/Camera.hpp"
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
	constexpr size_t LAUNCHER_ALLOCATOR_CAPACITY = 64 * 1'024 * 1'024;

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

	blk::Rect<unsigned> client_rect = {};

	BLK_IF_NOT_SUCCESS(blk::get_window_client_rect(client_rect))
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

	BLK_IF_NOT_SUCCESS(blk::create_renderer(client_rect))
	{
		BLK_FATAL("Failed to create the renderer\n");
	}

	BLK_IF_NOT_SUCCESS(blk::bake_renderer())
	{
		BLK_FATAL(" Failed to bake the renderer\n");
	}

	// ============================================================================
	// Game.
	// ============================================================================

	blk::Game_Context game_context = {};

	// `Game_Context` owns the world but `create_game` has no allocator to build it with, so we create it here and hand
	// it over already usable – with its pools, its hash map and its root node.
	BLK_IF_NOT_SUCCESS(blk::create_world(&allocator, game_context.world))
	{
		BLK_FATAL("Failed to create the game world\n");
	}

	BLK_IF_NOT_SUCCESS(blk::create_game(game_context))
	{
		BLK_FATAL("Failed to create the game\n");
	}

	// ============================================================================
	// Editor.
	// ============================================================================

	blk::Input_State input_state = {};

	// The editor reads the input state and spawns its own camera into the game world, so both have to exist first.
	blk::Editor_Context editor_context = {};

	BLK_IF_NOT_SUCCESS(blk::create_editor_context(&allocator, &input_state, &game_context.world, editor_context))
	{
		BLK_FATAL("Failed to create the editor context\n");
	}

	BLK_IF_NOT_SUCCESS(blk::create_editor(editor_context))
	{
		BLK_FATAL("Failed to create the editor\n");
	}

	// ============================================================================
	// Frame loop.
	// ============================================================================

	// `get_elapsed_seconds` reports the time since the timer was created, so the first delta is measured from here and
	// not from the start of the process.
	double previous_frame_seconds = 0.0;
	blk::get_elapsed_seconds(frame_timer, previous_frame_seconds);

	while (blk::is_application_running())
	{
		double current_frame_seconds = 0.0;
		blk::get_elapsed_seconds(frame_timer, current_frame_seconds);

		const double delta_time = current_frame_seconds - previous_frame_seconds;
		previous_frame_seconds = current_frame_seconds;

		blk::update_input(input_state);
		blk::update_editor(editor_context, delta_time);

		if (!editor_context.world_context.is_game_simulation_paused)
		{
			blk::update_game(game_context, delta_time, input_state);
		}

		// The window can be resized, so we sample the client rectangle every frame instead of reusing the one the
		// renderer was created with.
		BLK_IF_NOT_SUCCESS(blk::get_window_client_rect(client_rect))
		{
			// Without it we cannot build a projection matrix, so we skip the frame.
			continue;
		}

		if (client_rect.y == 0)
		{
			// The window is minimized. There is nothing to render, and the aspect ratio would divide by zero.
			continue;
		}

		// TODO (Feature): We always render the game world. `Editor_Mode::MATERIAL` has its own world and its own camera
		// in `editor_context.material_context`, and switching to that mode currently changes the widgets but not what
		// is on screen. The loop should pick the world and the camera from `editor_context.mode`.

		blk::Camera_View camera_view = {};

		if (const blk::Camera* camera = blk::get_camera(game_context.world, game_context.world.active_camera_handle))
		{
			const auto aspect_ratio = static_cast<float>(client_rect.x) / static_cast<float>(client_rect.y);

			camera_view.view = blk::init_view_matrix(game_context.world.scene_graph, *camera);
			camera_view.projection = blk::init_projection_matrix(*camera, aspect_ratio);

			if (const blk::Node* camera_node =
					blk::get_node(game_context.world, game_context.world.active_camera_handle))
			{
				camera_view.view_position = camera_node->transform.position;
			}
		}

		blk::update_frame(game_context.world.scene_graph, camera_view, game_context.world.settings);
		blk::render_frame(game_context.world.settings);
	}

	// ============================================================================
	// Teardown.
	// ============================================================================

	// The order is not free:
	//
	// - `destroy_editor` shuts the ImGui Vulkan backend down, so it has to run before `destroy_renderer`.
	// - `destroy_editor_context` despawns the editor camera from the game world, so it has to run before we destroy it.
	// - The resource storages outlive the world and the renderer, because destroying either one releases resources back
	//   into them.

	blk::destroy_editor();
	blk::destroy_editor_context(editor_context);

	blk::destroy_game(game_context);
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
