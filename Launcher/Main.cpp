#include "Core/Time.hpp"
#include "Editor/Console.hpp"
#include "Editor/Context.hpp"
#include "Editor/Editor.hpp"
#include "Game.hpp"
#include "Input/Input.hpp"
#include "Platform/Debug.hpp"
#include "Platform/Log.hpp"
#include "Platform/Window.hpp"
#include "Renderer/Renderer.hpp"

#include <Windows.h>
#include <imgui.h>

#include <chrono>

BLK_ENTRY()
{
	blk::create_log_sink(blk::log_debugger);
	blk::create_log_sink(blk::log_editor);

	blk::create_application(hInstance);
	blk::create_window();

	blk::Rect<int> client_size = blk::get_window_client_size();
	blk::create_renderer(client_size.x, client_size.y);

	blk::Game_Context game_context = {};
	blk::create_game(game_context);

	blk::Editor_Context editor_context = {};
	editor_context.world = &game_context.world;
	blk::create_editor(editor_context);

	blk::start_global_timer();
	double prev_frame_secs = blk::get_global_timer_seconds();

	blk::Input_State input_state = {};

	while (blk::is_application_running())
	{
		const double frame_secs = blk::get_global_timer_seconds();
		const double delta_time = frame_secs - prev_frame_secs;
		prev_frame_secs = frame_secs;

		blk::update_input_state(input_state);

		blk::update_editor(editor_context, delta_time, input_state);

		if (!editor_context.is_game_simulation_paused)
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
