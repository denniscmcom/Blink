// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Status_Bar.hpp"

#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Helpers.hpp"
#include "Engine/Editor/Stats.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/World/World.hpp"

#include <array>
#include <imgui.h>

namespace
{
void draw_horizontal_separator();
}  // namespace

void
blk::draw_status_bar(Editor_Context& context)
{
	BLK_CHECK(context.world);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	context.status_bar_size = Rect{.x = viewport->Size.x, .y = ImGui::GetFrameHeight()};

	context.status_bar_position.x = viewport->Pos.x;
	context.status_bar_position.y = viewport->Size.y - context.status_bar_size.y;

	context.status_bar_padding.x = ImGui::GetStyle().WindowPadding.x;
	context.status_bar_padding.y = (context.status_bar_size.y - ImGui::GetTextLineHeight()) * 0.5f;

	ImGui::SetNextWindowPos(to_imvec2(context.status_bar_position));
	ImGui::SetNextWindowSize(to_imvec2(context.status_bar_size));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, to_imvec2(context.status_bar_padding));

	if (ImGui::Begin(
			"##StatusBar",
			nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoBringToFrontOnFocus
		))
	{
		ImGui::Text("Blink v0.0.1");
		draw_horizontal_separator();
		ImGui::Text("Game: %s", context.is_game_simulation_paused ? "Paused" : "Running");
		draw_horizontal_separator();
		ImGui::Text(
			"Camera: %s",
			context.editor_camera_handle == context.world->active_camera_handle ? "Editor" : "Game"
		);
		draw_horizontal_separator();
		draw_fps();
		draw_horizontal_separator();
		draw_ms();
		draw_horizontal_separator();

		static std::array<std::pair<const char*, const char*>, 2> shortcuts = {
			{{"F1", "Pause/resume"}, {"F2", "Editor/game camera"}}
		};

		for (const auto& [key, description] : shortcuts)
		{
			ImGui::Text("%s - %s", key, description);
			draw_horizontal_separator();
		}
	}

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
