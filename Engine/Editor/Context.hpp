// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/World/World.hpp"

#include <imgui.h>

namespace blk
{
struct Input_State;
struct Camera;
struct World;

/// Minimum width for widgets on the left side of the screen.
constexpr float LEFT_COLUMN_MIN_WIDTH = 350.0f;
/// Maximum width for widgets on the left side of the screen.
constexpr float LEFT_COLUMN_MAX_WIDTH = 800.0f;

/// Minimum width for widgets on the right side of the screen.
constexpr float RIGHT_COLUMN_MIN_WIDTH = 350.0f;
/// Maximum width for widgets on the right side of the screen.
constexpr float RIGHT_COLUMN_MAX_WIDTH = 800.0f;

/// Global editor mode. Each mode has its own set of widgets and functionality.
enum class Editor_Mode
{
	WORLD,
	MATERIAL,
	PREFAB,
};

/// Context for `Editor_Mode::WORLD`.
struct Editor_World_Context
{
	/// A pointer to the in-game world.
	/// It is needed to modify the game world using the editor – like spawning entities.
	World* world;

	/// Handle to the in-editor camera.
	Pool_Handle<Camera> editor_camera_handle;
	/// The game camera is owned by the game; The editor only keeps a reference to it to restore it later.
	Pool_Handle<Camera> game_camera_handle;

	/// Game simulation is paused.
	bool is_game_simulation_paused = true;
	/// Editor camera is active.
	/// In this mode, the in-game or editor camera could be active.
	bool is_editor_camera_active = true;
};

/// Context for `Editor_Mode::MATERIAL`.
struct Editor_Material_Context
{
	/// Handle to the editor camera.
	Pool_Handle<Camera> camera_handle;

	/// The world to author the material.
	World world;

	/// Material settings widget position.
	ImVec2 material_settings_position;
	/// Material settings widget size.
	ImVec2 material_settings_size;
};

/// Context for `Editor_Mode::PREFAB`.
struct Editor_Prefab_Context
{
	/// Handle to the editor camera.
	Pool_Handle<Camera> camera_handle;

	/// The world to author the prefab.
	World world;
};

struct Editor_Context
{
	/// Pointer to the editor allocator.
	Allocator* allocator;

	/// User input state.
	const Input_State* input_state;

	/// Current editor mode.
	Editor_Mode mode = Editor_Mode::WORLD;
	/// Active world based on `mode`.
	World* active_world;

	/// Pointer to main editor font.
	ImFont* main_font;

	/// Current node selected in the scene graph.
	Pool_Handle<Node> selected_node_handle;

	/// Default width for widgets on the left side of the screen.
	float left_column_width = 550.0f;
	/// Default width for widgets on the right side of the screen.
	float right_column_width = 650.0f;

	/// In-editor camera is in free fly mode.
	bool is_in_free_fly_mode = false;
	/// Cursor x position before entering free fly.
	int cursor_position_x;
	/// Cursor y position before entering free fly.
	int cursor_position_y;

	/// Toolbar widget is visible.
	bool is_showing_toolbar = true;
	/// Scene graph and settings widgets are visible.
	bool is_showing_scene_graph = true;
	/// Settings widget is visible.
	bool is_showing_settings = true;
	/// Console widget is visible.
	bool is_showing_console = true;
	/// Statistics widget is visible.
	bool is_showing_stats = true;

	/// Show triangle meshes toggle.
	bool is_showing_triangles;

	/// Toolbar widget position.
	ImVec2 toolbar_position;
	/// Toolbar widget size.
	ImVec2 toolbar_size;

	/// Console widget position.
	ImVec2 console_position;
	/// Console widget size.
	ImVec2 console_size;

	/// Scene graph widget position.
	ImVec2 scene_graph_position;
	/// Scene graph widget size.
	ImVec2 scene_graph_size;
	/// Scene graph widget minimum size.
	ImVec2 scene_graph_min_size;
	/// Scene graph widget maximum size.
	ImVec2 scene_graph_max_size;

	/// Settings widget position.
	ImVec2 settings_position;
	/// Settings widget size.
	ImVec2 settings_size;
	/// Settings widget minimum size.
	ImVec2 settings_min_size;
	/// Settings widget maximum size.
	ImVec2 settings_max_size;

	/// Status bar widget position.
	ImVec2 status_bar_position;
	/// Status bar widget size.
	ImVec2 status_bar_size;
	/// Status bar widget padding.
	ImVec2 status_bar_padding;

	/// Statistics widget position.
	ImVec2 stats_position;
	/// Statistics widget size.
	ImVec2 stats_size;
	/// Statistics widget minimum size.
	ImVec2 stats_min_size;
	/// Statistics widget maximum size.
	ImVec2 stats_max_size;

	/// Context for `Editor_Mode::WORLD`.
	Editor_World_Context world_context;
	/// Context for `Editor_Mode::MATERIAL`.
	Editor_Material_Context material_context;
	/// Context for `Editor_Mode::PREFAB`.
	Editor_Prefab_Context prefab_context;
};
}  // namespace blk
