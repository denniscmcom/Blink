// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Application.hpp"

namespace blk
{
struct Node;
struct World;
struct Camera;

constexpr float LEFT_COLUMN_MIN_WIDTH = 350.0f;
constexpr float LEFT_COLUMN_MAX_WIDTH = 800.0f;

constexpr float RIGHT_COLUMN_MIN_WIDTH = 350.0f;
constexpr float RIGHT_COLUMN_MAX_WIDTH = 800.0f;

struct Editor_Context
{
	World* world;

	Pool_Handle<Camera> editor_camera_handle;
	// The game camera is owned by the game; The editor only keeps a reference to it to restore it later.
	Pool_Handle<Camera> game_camera_handle;
	Pool_Handle<Node> selected_node_handle;

	bool is_in_free_fly_mode;
	bool is_editor_camera_active = true;
	bool is_game_simulation_paused = true;

	bool show_outliner = true;
	bool show_stats = true;
	bool show_console = true;
	bool show_material_creator = false;

	float left_column_width = 550.0f;
	float right_column_width = 650.0f;

	Rect<int> cursor_position_before_hidden;

	Rect<float> scene_graph_position;
	Rect<float> scene_graph_size;
	Rect<float> scene_graph_min_size;
	Rect<float> scene_graph_max_size;

	Rect<float> settings_position;
	Rect<float> settings_size;
	Rect<float> settings_min_size;
	Rect<float> settings_max_size;

	Rect<float> stats_position;
	Rect<float> stats_size;
	Rect<float> stats_min_size;
	Rect<float> stats_max_size;

	Rect<float> status_bar_position;
	Rect<float> status_bar_size;
	Rect<float> status_bar_padding;

	Rect<float> console_position;
	Rect<float> console_size;

	Rect<float> material_creator_position;
	Rect<float> material_creator_size;
};
}  // namespace blk
