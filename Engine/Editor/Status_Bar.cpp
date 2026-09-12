// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Status_Bar.hpp"

#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/World/Stats.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/World/World.hpp"

#include <imgui.h>

namespace
{
/// Helper function to draw an horizontal separator.
void draw_horizontal_separator();
}  // namespace

void
blk::draw_status_bar(Editor_Context& context)
{
	if (!BLK_VERIFY(context.world_context.game_world) || !BLK_VERIFY(context.input_state))
	{
		return;
	}

	// Get the ImGui viewport.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Compute the status bar size and position.
	// The status bar should fill the entire width of the viewport. It's height is just enough to fit text.

	context.viewport_context.status_bar_size = ImVec2(viewport->WorkSize.x, ImGui::GetFrameHeight());

	context.viewport_context.status_bar_position.x = viewport->WorkPos.x;
	context.viewport_context.status_bar_position.y =
		viewport->WorkPos.y + viewport->WorkSize.y - context.viewport_context.status_bar_size.y;

	context.viewport_context.status_bar_padding.x = ImGui::GetStyle().WindowPadding.x;
	context.viewport_context.status_bar_padding.y =
		(context.viewport_context.status_bar_size.y - ImGui::GetTextLineHeight()) * 0.5f;

	// Pass size and position information to ImGui.

	ImGui::SetNextWindowPos(context.viewport_context.status_bar_position);
	ImGui::SetNextWindowSize(context.viewport_context.status_bar_size);

	// Set padding.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, context.viewport_context.status_bar_padding);

	// The status bar is not movable by the user, should not have any decorations and it is always visible.

	ImGui::Begin(
		"##StatusBar",
		nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoBringToFrontOnFocus
	);

	ImGui::Text("Blink " BLK_ENGINE_VERSION);
	draw_horizontal_separator();

	// Depending on `Editor_Mode` we display context-based information.

	switch (context.mode)
	{
	case Editor_Mode::WORLD: {
		// The F1 and F2 shortcuts these lines report on are handled by `update_editor`. This widget only reads state.

		// Display whether the game is paused or running.
		ImGui::Text("Game: %s", context.world_context.is_game_simulation_paused ? "Paused" : "Running");

		draw_horizontal_separator();

		// Display which camera is used; editor or in-game.
		ImGui::Text(
			"Camera: %s",
			context.world_context.editor_camera_handle == context.world_context.game_world->active_camera_handle
				? "Editor"
				: "Game"
		);

		draw_horizontal_separator();

		// Show most used shortcuts.

		ImGui::Text("F1 - Pause/resume");
		draw_horizontal_separator();
		ImGui::Text("F2 - Editor/game camera");
	}
	break;
	case Editor_Mode::MATERIAL: {
	}
	break;
	}

	// Display some performance data.

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
