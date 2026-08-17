// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/World/World.hpp"

#include <imgui.h>

namespace blk
{
struct Node;
struct World;
struct Camera;

/// Minimum width for widgets on the left side of the screen.
constexpr float LEFT_COLUMN_MIN_WIDTH = 350.0f;
/// Maximum width for widgets on the left side of the screen.
constexpr float LEFT_COLUMN_MAX_WIDTH = 800.0f;

/// Minimum width for widgets on the right side of the screen.
constexpr float RIGHT_COLUMN_MIN_WIDTH = 350.0f;
/// Maximum width for widgets on the right side of the screen.
constexpr float RIGHT_COLUMN_MAX_WIDTH = 800.0f;

/// Global editor mode. Each mode has it's own set of widgets and functionality.
enum class Editor_Mode
{
	WORLD,
	MATERIAL,
};

/// Context for `Editor_Mode::WORLD`.
struct World_Context
{
	explicit World_Context(World* game_world);

	World* _game_world;

	/// Handle to the in-editor camera.
	Pool_Handle<Camera> editor_camera_handle = {};
	/// The game camera is owned by the game; The editor only keeps a reference to it to restore it later.
	Pool_Handle<Camera> game_camera_handle = {};
	/// Current node selected in `Editor_Mode::WORLD`.
	Pool_Handle<Node> selected_node_handle = {};

	/// Game simulation is paused.
	bool is_game_simulation_paused = true;
	/// In-editor camera is active.
	bool is_editor_camera_active = true;

	/// Scene graph and settings widgets are visible.
	bool show_outliner = true;
	/// Statistics widget is visible.
	bool show_stats = true;
	/// Console widget is visible.
	bool show_console = true;

	/// Scene graph widget position.
	ImVec2 scene_graph_position = {};
	/// Scene graph widget size.
	ImVec2 scene_graph_size = {};
	/// Scene graph widget minimum size.
	ImVec2 scene_graph_min_size = {};
	/// Scene graph widget maximum size.
	ImVec2 scene_graph_max_size = {};

	/// Settings widget position.
	ImVec2 settings_position = {};
	/// Settings widget size.
	ImVec2 settings_size = {};
	/// Settings widget minimum size.
	ImVec2 settings_min_size = {};
	/// Settings widget maximum size.
	ImVec2 settings_max_size = {};

	/// Statistics widget position.
	ImVec2 stats_position = {};
	/// Statistics widget size.
	ImVec2 stats_size = {};
	/// Statistics widget minimum size.
	ImVec2 stats_min_size = {};
	/// Statistics widget maximum size.
	ImVec2 stats_max_size = {};

	/// Console widget position.
	ImVec2 console_position = {};
	/// Console widget size.
	ImVec2 console_size = {};
};

/// Activates editor camera for `Editor_Mode::WORLD` if it's not already active.
void activate_world_mode_editor_camera(World_Context& context);

/// Context for `Editor_Mode::MATERIAL`.
struct Material_Context
{
	explicit Material_Context();

	/// Handle to the editor camera.
	Pool_Handle<Camera> camera_handle;
	/// Handle to a prop to preview the material on.
	Pool_Handle<Prop> material_prop_handle;

	World world;

	/// Material settings widget is visible.
	bool show_material_settings = true;

	/// Material settings widget position.
	ImVec2 material_settings_position = {};
	/// Material settings widget size.
	ImVec2 material_settings_size = {};
};

/// Global context for the viewport.
struct Viewport_Context
{
	/// In-editor camera is in free fly mode.
	bool is_in_free_fly_mode = false;

	/// Cursor position before entering free fly. When we enter free fly the cursor is hidden, so we use this value to
	/// restore it's position after exiting free fly.
	Rect<int> cursor_position_before_hidden = {};

	// The status bar widget settings are the same across `Editor_Mode`, only the content changes.

	/// Status bar widget position.
	ImVec2 status_bar_position = {};
	/// Status bar widget size.
	ImVec2 status_bar_size = {};
	/// Status bar widget padding.
	ImVec2 status_bar_padding = {};
};

/// Global context for the editor.
struct Editor_Context
{
	explicit Editor_Context(const Input_State* input_state, World* game_world);

	/// User input state.
	const Input_State* _input_state;

	/// Current editor mode.
	Editor_Mode mode = Editor_Mode::WORLD;

	/// Pointer to main editor font.
	ImFont* main_font = nullptr;

	/// Toolbar widget is visible.
	bool show_toolbar = true;

	/// Default width for widgets on the left side of the screen.
	float left_column_width = 550.0f;
	/// Default width for widgets on the right side of the screen.
	float right_column_width = 650.0f;

	// The toolbar widget settings are the same across `Editor_Mode`, only the content changes.

	/// Toolbar widget position.
	ImVec2 toolbar_position = {};
	/// Toolbar widget size.
	ImVec2 toolbar_size = {};

	/// Global viewport context.
	Viewport_Context viewport_context = {};
	/// Context for `Editor_Mode::WORLD`.
	World_Context world_context;
	/// Context for `Editor_Mode::MATERIAL`.
	Material_Context material_context = Material_Context();
};
}  // namespace blk
