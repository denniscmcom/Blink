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
#include <tracy/Tracy.hpp>

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
		blk::Editor_Context editor_context = blk::update_editor(input_state, delta_time);

		if (!blk::is_game_simulation_paused())
		{
			ZoneScopedN("Update game");
			blk::game::update_game(game_context, delta_time, input_state);
		}

		blk::World* active_world = blk::get_active_world();
		blk::update_world(*active_world);

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

		// Initialize frame descriptor.
		blk::Frame_Descriptor frame_descriptor = {};

		if (const blk::Camera* camera = blk::get_camera(*active_world, active_world->active_camera_handle))
		{
			const auto aspect_ratio = static_cast<float>(client_size_x) / static_cast<float>(client_size_y);

			frame_descriptor.view = blk::init_view_matrix(active_world->scene_graph, *camera);
			frame_descriptor.projection = blk::init_projection_matrix(*camera, aspect_ratio);

			if (const blk::Node* camera_node = blk::get_node(active_world->scene_graph, camera->node_handle))
			{
				const blk::Vector4& camera_position = camera_node->world_matrix.columns[3];
				frame_descriptor.view_position = {camera_position.x, camera_position.y, camera_position.z};
			}
		}

		const blk::Atmosphere_Settings& atmosphere = active_world->settings.atmosphere;

		if (blk::Node* sun = get_node(active_world->scene_graph, atmosphere.sun_node_handle))
		{
			// `begin_render_frame` rebuilds the rotation as `{elevation, azimuth, 0}`, so elevation is the `x` (pitch)
			// component and azimuth the `y` (yaw) one.
			frame_descriptor.sun_azimuth = blk::Radians{sun->transform.rotation.y};
			frame_descriptor.sun_elevation = blk::Radians{sun->transform.rotation.x};
			frame_descriptor.sun_radius = blk::to_radians(blk::Degrees{sun->transform.scale.x});
			frame_descriptor.sun_intensity = sun->directional_light.intensity;
		}

		frame_descriptor.ground_albedo = {
			atmosphere.ground_albedo.r,
			atmosphere.ground_albedo.g,
			atmosphere.ground_albedo.b,
		};

		frame_descriptor.planet_radius = blk::Radians{atmosphere.planet_radius};
		frame_descriptor.atmosphere_radius = blk::Radians{atmosphere.atmosphere_radius};

		frame_descriptor.rayleigh_scattering = atmosphere.rayleigh_scattering;
		frame_descriptor.rayleigh_scale = atmosphere.rayleigh_scale;

		frame_descriptor.ozone_absorption = atmosphere.ozone_absorption;
		frame_descriptor.ozone_mid_point = atmosphere.ozone_mid_point;
		frame_descriptor.ozone_low_point = atmosphere.ozone_low_point;

		frame_descriptor.mie_scattering = atmosphere.mie_scattering;
		frame_descriptor.mie_absorption = atmosphere.mie_absorption;
		frame_descriptor.mie_asymmetry = atmosphere.mie_asymmetry;
		frame_descriptor.mie_scale = atmosphere.mie_scale;

		const blk::Terrain_Settings& terrain = active_world->settings.terrain;

		frame_descriptor.terrain_heightmap_handle = terrain.heightmap_handle;
		frame_descriptor.terrain_chunk_size = blk::compute_chunk_size(terrain);
		frame_descriptor.terrain_height = terrain.height;
		frame_descriptor.skirt_depth = terrain.skirt_depth;

#ifdef BLK_DEBUG_TARGET
		frame_descriptor.is_debug_triangle_enabled = editor_context.active_viewport_toggles->is_showing_triangles;
		frame_descriptor.is_skybox_transmittance_enabled = atmosphere.enable_transmittance;
		frame_descriptor.is_skybox_multiscattering_enabled = atmosphere.enable_multiscattering;
		frame_descriptor.is_sky_view_enabled = atmosphere.enable_sky_view;
		frame_descriptor.is_skybox_aerial_enabled = atmosphere.enable_aerial;
#endif

		// Render.

		blk::begin_render_frame(frame_descriptor);

		if (editor_context.active_viewport_toggles->is_showing_world_axis)
		{
			blk::draw_world_axis();
		}

		if (editor_context.active_viewport_toggles->is_showing_physics_world)
		{
			blk::draw_physics_debug(active_world->physics);
		}

		for (size_t i = 0; i < active_world->scene_graph.nodes.capacity; ++i)
		{
			const blk::Pool_Slot<blk::Node>& slot = active_world->scene_graph.nodes.slots[i];

			if (!slot.is_used)
			{
				continue;
			}

			const blk::Node& node = slot.element;

			switch (node.type)
			{
			case blk::Node_Type::SPATIAL: {
				// Nothing to do.
			}
			break;
			case blk::Node_Type::MESH_REF: {
				const blk::Mesh_Descriptor descriptor = {
					.world_matrix = node.world_matrix,
					.mesh_handle = node.mesh_ref.mesh_handle,
					.material_handle = node.mesh_ref.material_handle,
				};

				blk::draw_mesh(descriptor);
			}
			break;
			case blk::Node_Type::POINT_LIGHT: {
				// Nothing yet.
			}
			break;
			case blk::Node_Type::DIRECTIONAL_LIGHT: {
				// Nothing yet. The sun is handled by the frame descriptor.
			}
			break;
			case blk::Node_Type::TERRAIN: {
				const blk::Terrain_Chunk_Descriptor descriptor = {
					.world_matrix = node.world_matrix,
					.grid_handle = active_world->settings.terrain.chunk_grid_handles.buffer[node.terrain_ref.lod],
					// Terrain has no material yet, so it renders with the fallback one.
					.material_handle = {},
					.uv_offset = node.terrain_ref.uv_offset,
					.uv_scale = node.terrain_ref.uv_scale,
					.lod = node.terrain_ref.lod,
				};

				blk::draw_terrain_chunk(descriptor);
			}
			break;
			case blk::Node_Type::STATIC_BODY: {
				// Nothing yet.
			}
			break;
			case blk::Node_Type::CAPSULE_COLLIDER: {
				// Nothing yet.
			}
			break;
			}
		}

		blk::end_render_frame();

		// Tracy: This should be at the end of the frame loop.
		FrameMark;
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
