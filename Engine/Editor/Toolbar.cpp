// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Toolbar.hpp"

#include "Engine/Editor/Context.hpp"
#include "Engine/Scene/Scene_Graph.hpp"
#include "Engine/World/World.hpp"

#include <IconsMaterialDesign.h>
#include <imgui_internal.h>

#include <array>
#include <string>

namespace
{
bool draw_button(const char* icon, const char* tooltip, const ImVec2& size);
void draw_separator();
}  // namespace

void
blk::draw_toolbar(Editor_Context& context)
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	context.toolbar_position = viewport->WorkPos;
	context.toolbar_size.x = viewport->WorkSize.x;
	context.toolbar_size.y = 80.0f;

	ImGui::SetNextWindowPos(context.toolbar_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.toolbar_size, ImGuiCond_Always);

	ImVec4 popup_bg = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
	popup_bg.w = 1.0f;
	ImGui::PushStyleColor(ImGuiCol_PopupBg, popup_bg);

	ImGui::Begin(
		"##Toolbar",
		nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoBringToFrontOnFocus
	);

	ImGui::SetNextItemWidth(400.0f);

	if (constexpr std::array mode_labels = {"World", "Material"};
		ImGui::BeginCombo("##Mode", mode_labels.at(static_cast<size_t>(context.mode))))
	{
		for (size_t i = 0; i < mode_labels.size(); i++)
		{
			bool is_mode_active = static_cast<size_t>(context.mode) == i;

			if (ImGui::Selectable(mode_labels.at(i), is_mode_active))
			{
				context.mode = static_cast<Editor_Mode>(i);
			}

			if (is_mode_active)
			{
			}
		}

		ImGui::EndCombo();
	}

	ImGui::PopStyleColor();

	const auto button_size = ImVec2(80.0f, ImGui::GetContentRegionAvail().y);
	draw_separator();

	switch (context.mode)
	{
	case Editor_Mode::WORLD: {
		if (draw_button(ICON_MD_FOLDER "", "Load world", button_size))
		{
		}

		ImGui::SameLine();

		if (draw_button(ICON_MD_SAVE "", "Save world", button_size))
		{
		}

		draw_separator();

		if (draw_button(ICON_MD_PERSON "", "Spawn an actor", button_size))
		{
			const std::string actor_name =
				make_unique_node_name(context.world_context._game_world->scene_graph, "Actor");
			spawn_actor(
				*context.world_context._game_world,
				actor_name.c_str(),
				context.world_context.selected_node_handle
			);
		}

		ImGui::SameLine();

		if (draw_button(ICON_MD_PHOTO_CAMERA "", "Spawn a camera", button_size))
		{
			const std::string camera_name =
				make_unique_node_name(context.world_context._game_world->scene_graph, "Camera");
			spawn_actor(
				*context.world_context._game_world,
				camera_name.c_str(),
				context.world_context.selected_node_handle
			);
		}

		ImGui::SameLine();

		if (draw_button(ICON_MD_LIGHTBULB "", "Spawn a point light", button_size))
		{
		}

		ImGui::SameLine();

		if (draw_button(ICON_MD_CHAIR "", "Spawn a prop", button_size))
		{
			const std::string prop_name = make_unique_node_name(context.world_context._game_world->scene_graph, "Prop");
			spawn_actor(
				*context.world_context._game_world,
				prop_name.c_str(),
				context.world_context.selected_node_handle
			);
		}

		draw_separator();

		if (draw_button(ICON_MD_DELETE "", "Despawn selected node", button_size))
		{
			despawn_node(*context.world_context._game_world, context.world_context.selected_node_handle);
			context.world_context.selected_node_handle = {};
		}
	}
	break;
	case Editor_Mode::MATERIAL: {
		if (draw_button(ICON_MD_FOLDER "", "Load material", button_size))
		{
		}

		ImGui::SameLine();

		if (draw_button(ICON_MD_SAVE "", "Save material", button_size))
		{
		}
	}
	break;
	}

	ImGui::End();
}

namespace
{
bool
draw_button(const char* icon, const char* tooltip, const ImVec2& size)
{
	if (ImGui::Button(icon, size))
	{
		return true;
	}

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
	{
		ImGui::SetTooltip(tooltip);
	}

	return false;
}

void
draw_separator()
{
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 3);
	ImGui::SameLine();
}
}  // namespace
