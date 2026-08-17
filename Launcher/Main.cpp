// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "../Engine/Editor/World/Console.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Editor.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Debug.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/Scene/Scene_Graph.hpp"
#include "Engine/World/Camera.hpp"
#include "Game/Game.hpp"

#include <Windows.h>
#include <imgui.h>

#include <chrono>

BLK_ENTRY()
{
	blk::create_log_sink(blk::log_debugger);
	blk::create_log_sink(blk::log_editor);

	blk::create_application(hInstance);
	blk::create_window(BLK_PROJECT_NAME);

	const blk::Rect<unsigned> client_size = blk::get_window_client_size();
	blk::create_renderer(client_size);

	blk::Game_Context game_context = {};
	blk::create_game(game_context);

	blk::Input_State input_state = {};

	blk::Editor_Context editor_context(&input_state, &game_context.world);
	blk::create_editor(editor_context);

	blk::start_global_timer();
	double prev_frame_secs = blk::get_global_timer_seconds();

	while (blk::is_application_running())
	{
		const double frame_secs = blk::get_global_timer_seconds();
		const double delta_time = frame_secs - prev_frame_secs;
		prev_frame_secs = frame_secs;

		blk::update_input_state(input_state);
		blk::update_editor(editor_context, delta_time);

		if (!editor_context.world_context.is_game_simulation_paused)
		{
			blk::update_game(game_context, delta_time, input_state);
		}

		blk::Camera_View camera_view = {};

		if (const blk::Camera* camera = game_context.world.cameras.get(game_context.world.active_camera_handle))
		{
			camera_view.view = blk::make_view_matrix(game_context.world.scene_graph, *camera);
			const auto aspect_ratio = static_cast<float>(client_size.x) / static_cast<float>(client_size.y);
			camera_view.projection = blk::make_projection_matrix(*camera, aspect_ratio);

			if (const blk::Node* camera_node = game_context.world.scene_graph.nodes.get(camera->node_handle))
			{
				camera_view.view_position = camera_node->transform.position;
			}
		}

		blk::update_frame(game_context.world.scene_graph, camera_view);
		blk::render_frame();
	}

	blk::destroy_editor();
	blk::destroy_game(game_context);
	blk::destroy_renderer();
	blk::destroy_window();
}
