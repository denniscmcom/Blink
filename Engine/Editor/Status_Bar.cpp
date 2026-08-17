// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Status_Bar.hpp"

#include "Engine/Editor/Camera.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/World/World.hpp"
#include "World/Stats.hpp"

#include <array>
#include <imgui.h>

namespace
{
void draw_horizontal_separator();
}  // namespace

void
blk::draw_status_bar(Editor_Context& context)
{
	BLK_CHECK(context.world_context._game_world);
	BLK_CHECK(context._input_state);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	context.viewport_context.status_bar_size = ImVec2(viewport->Size.x, ImGui::GetFrameHeight());

	context.viewport_context.status_bar_position.x = viewport->Pos.x;
	context.viewport_context.status_bar_position.y = viewport->Size.y - context.viewport_context.status_bar_size.y;

	context.viewport_context.status_bar_padding.x = ImGui::GetStyle().WindowPadding.x;
	context.viewport_context.status_bar_padding.y =
		(context.viewport_context.status_bar_size.y - ImGui::GetTextLineHeight()) * 0.5f;

	ImGui::SetNextWindowPos(context.viewport_context.status_bar_position);
	ImGui::SetNextWindowSize(context.viewport_context.status_bar_size);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, context.viewport_context.status_bar_padding);

	static constexpr std::array<std::pair<const char*, const char*>, 2> world_mode_shortcuts = {
		{{"F1", "Pause/resume"}, {"F2", "Editor/game camera"}}
	};

	ImGui::Begin(
		"##StatusBar",
		nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoBringToFrontOnFocus
	);

	ImGui::Text("Blink " BLK_ENGINE_VERSION);
	draw_horizontal_separator();

	switch (context.mode)
	{
	case Editor_Mode::WORLD: {
		// Pause/resume game simulation.
		if (is_key_press(*context._input_state, Key::KEYBOARD_F1))
		{
			context.world_context.is_game_simulation_paused = !context.world_context.is_game_simulation_paused;
		}

		// Editor/game camera.
		if (is_key_press(*context._input_state, Key::KEYBOARD_F2))
		{
			if (context.world_context.is_editor_camera_active)
			{
				context.world_context._game_world->active_camera_handle = context.world_context.game_camera_handle;
				context.world_context.is_editor_camera_active = false;
			}
			else
			{
				activate_world_mode_editor_camera(context.world_context);
			}
		}

		ImGui::Text("Game: %s", context.world_context.is_game_simulation_paused ? "Paused" : "Running");
		draw_horizontal_separator();
		ImGui::Text(
			"Camera: %s",
			context.world_context.editor_camera_handle == context.world_context._game_world->active_camera_handle
				? "Editor"
				: "Game"
		);

		draw_horizontal_separator();

		for (const auto& [key, description] : world_mode_shortcuts)
		{
			ImGui::Text("%s - %s", key, description);
			draw_horizontal_separator();
		}
	}
	break;
	case Editor_Mode::MATERIAL: {
	}
	break;
	}

	draw_horizontal_separator();
	draw_fps();
	draw_horizontal_separator();
	draw_ms();
	draw_horizontal_separator();

	ImGui::End();
	ImGui::PopStyleVar();
}

namespace
{
void
draw_horizontal_separator()
{
	ImGui::SameLine();
	ImGui::TextUnformatted("|");
	ImGui::SameLine();
}
}  // namespace
