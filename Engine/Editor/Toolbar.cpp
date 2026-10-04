// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Toolbar.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/World.hpp"

#include <IconsMaterialDesign.h>
#include <imgui_internal.h>

namespace
{
/// Helper to draw a toolbar icon button.
bool draw_button(const char* icon, const char* tooltip, const ImVec2& size);
/// Helper to draw a separator in the toolbar.
void draw_separator();
/// Draws buttons to spawn each entity type.
void draw_spawn_entity_buttons(blk::Editor_Context& context, const ImVec2& button_size);
/// Draws a button to spawn a prefab from a file.
void draw_load_prefab_button(blk::Editor_Context& context, const ImVec2& button_size);
/// Helper to draw a toolbar icon button that stays highlighted while `is_active` is set.
bool draw_toggle_button(const char* icon, const char* tooltip, bool& is_active, const ImVec2& size);
}  // namespace

void
blk::draw_toolbar(Editor_Context& context)
{
	// Get main ImGui viewport.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Compute toolbar position and size.
	// The toolbar is located below the main menu and it should be full width. The ImGui viewport already takes into
	// account the height of the main menu, so we do not have to derive it manually to position the toolbar correctly.
	// This is the reason why the main menu should be drawn first.

	context.toolbar_position = viewport->WorkPos;
	context.toolbar_size.x = viewport->WorkSize.x;
	context.toolbar_size.y = 80.0f;

	// Pass position and size to ImGui.

	ImGui::SetNextWindowPos(context.toolbar_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.toolbar_size, ImGuiCond_Always);

	// Remove transparency from background.

	ImVec4 popup_bg = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
	popup_bg.w = 1.0f;
	ImGui::PushStyleColor(ImGuiCol_PopupBg, popup_bg);

	// The toolbar should be non-movable by the user and with no decorations.

	ImGui::Begin(
		"##Toolbar",
		nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoBringToFrontOnFocus
	);

	// This is the width for the `Editor_Mode` switch.
	ImGui::SetNextItemWidth(400.0f);

	// Current modes supported.
	// When new editor modes are added to `Editor_Mode`, we have to implement them here too.
	// `mode_labels` should be in the same order as `Editor_Mode` because we use enum indexing later.
	constexpr Array mode_labels = {{
		"World",
		"Material",
		"Prefab",
	}};

	// We draw the switch with the current mode.
	if (ImGui::BeginCombo("##Mode", mode_labels.buffer[static_cast<size_t>(context.mode)]))
	{
		// Iterate over all other modes to display them when the user wants to switch.
		for (size_t mode_index = 0; mode_index < mode_labels.capacity; mode_index++)
		{
			// If `mode_index` is selected.
			if (const bool is_mode_active = static_cast<size_t>(context.mode) == mode_index;
				ImGui::Selectable(mode_labels.buffer[mode_index], is_mode_active))
			{
				// Switch to that mode.
				context.mode = static_cast<Editor_Mode>(mode_index);
			}
		}

		ImGui::EndCombo();
	}

	ImGui::PopStyleColor();

	// We finish the mode selector, now we draw the context-dependent buttons.

	// This is the button size for all following buttons.
	const auto button_size = ImVec2(80.0f, ImGui::GetContentRegionAvail().y);

	draw_separator();

	switch (context.mode)
	{
	case Editor_Mode::WORLD: {
		// Load a world from file.

		if (draw_button(ICON_MD_FOLDER "", "Load world", button_size))
		{
			BLK_NOT_IMPLEMENTED();
		}

		ImGui::SameLine();

		// Save a world to a file.

		if (draw_button(ICON_MD_SAVE "", "Save world", button_size))
		{
			BLK_NOT_IMPLEMENTED();
		}

		draw_separator();

		// Load prefab from file.
		draw_load_prefab_button(context, button_size);
		ImGui::SameLine();

		// Draw buttons to spawn entities.
		draw_spawn_entity_buttons(context, button_size);

		ImGui::SameLine();
	}
	break;
	case Editor_Mode::MATERIAL: {
		// Load a material from file and display it in the current `Mesh_Instance`.

		if (draw_button(ICON_MD_FOLDER "", "Load material", button_size))
		{
			BLK_NOT_IMPLEMENTED();
		}

		ImGui::SameLine();

		// Save a material to a file.

		if (draw_button(ICON_MD_SAVE "", "Save material", button_size))
		{
			BLK_NOT_IMPLEMENTED();
		}

		draw_separator();

		// Load prefab from file.
		draw_load_prefab_button(context, button_size);

		ImGui::SameLine();

		if (draw_button(ICON_MD_CIRCLE "", "Spawn UV sphere", button_size))
		{
			BLK_NOT_IMPLEMENTED();
		}
	}
	break;
	case Editor_Mode::PREFAB: {
		// Load a prefab from file.
		draw_load_prefab_button(context, button_size);

		ImGui::SameLine();

		// Save a prefab to a file.

		if (draw_button(ICON_MD_SAVE "", "Save prefab", button_size))
		{
			BLK_NOT_IMPLEMENTED();
		}

		draw_separator();

		// Draw buttons to spawn entities.
		draw_spawn_entity_buttons(context, button_size);
	}
	break;
	}

	// Viewport options.

	draw_separator();

	draw_toggle_button(ICON_MD_GRID_ON "", "Show triangles", context.is_showing_triangles, button_size);

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
		ImGui::SetTooltip("%s", tooltip);
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

void
draw_spawn_entity_buttons(blk::Editor_Context& context, const ImVec2& button_size)
{
	// Spawns an actor into the game world.

	if (draw_button(ICON_MD_PERSON "", "Spawn an actor", button_size))
	{
		if (spawn_actor(*context.active_world, context.selected_node_handle) == blk::POOL_HANDLE_NONE<blk::Actor>)
		{
			BLK_ERROR("Failed to spawn actor\n");
		}
	}

	ImGui::SameLine();

	// Spawns a camera into the game world.

	if (draw_button(ICON_MD_PHOTO_CAMERA "", "Spawn a camera", button_size))
	{
		if (spawn_camera(*context.active_world, context.selected_node_handle) == blk::POOL_HANDLE_NONE<blk::Camera>)
		{
			BLK_ERROR("Failed to spawn camera\n");
		}
	}

	ImGui::SameLine();

	// Spawns a point light into the game world.

	if (draw_button(ICON_MD_LIGHTBULB "", "Spawn a point light", button_size))
	{
		if (spawn_node(context.active_world->scene_graph, blk::Node_Type::POINT_LIGHT, context.selected_node_handle) ==
			blk::POOL_HANDLE_NONE<blk::Node>)
		{
			BLK_ERROR("Failed to spawn point light\n");
		}
	}

	ImGui::SameLine();

	// Spawns a directional light into the game world.

	if (draw_button(ICON_MD_SUNNY "", "Spawn a directional light", button_size))
	{
		if (spawn_node(
				context.active_world->scene_graph,
				blk::Node_Type::DIRECTIONAL_LIGHT,
				context.selected_node_handle
			) == blk::POOL_HANDLE_NONE<blk::Node>)
		{
			BLK_ERROR("Failed to spawn directional light\n");
		}
	}

	ImGui::SameLine();

	// Spawns a prop into the game world.

	if (draw_button(ICON_MD_CHAIR "", "Spawn a prop", button_size))
	{
		if (spawn_prop(*context.active_world, context.selected_node_handle) == blk::POOL_HANDLE_NONE<blk::Prop>)
		{
			BLK_ERROR("Failed to spawn prop\n");
		}
	}

	draw_separator();

	// Despawns the current selected node in the outliner.

	if (draw_button(ICON_MD_DELETE "", "Despawn selected node", button_size))
	{
		// TODO (Feature): not implemented.
		BLK_NOT_IMPLEMENTED();
	}
}

void
draw_load_prefab_button(blk::Editor_Context& context, const ImVec2& button_size)
{
	if (draw_button(ICON_MD_CONSTRUCTION "", "Load prefab", button_size))
	{
		BLK_NOT_IMPLEMENTED();
	}
}

bool
draw_toggle_button(const char* icon, const char* tooltip, bool& is_active, const ImVec2& size)
{
	const bool was_active = is_active;

	if (was_active)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	}

	const bool is_pressed = draw_button(icon, tooltip, size);

	if (was_active)
	{
		ImGui::PopStyleColor();
	}

	if (is_pressed)
	{
		is_active = !is_active;
	}

	return is_pressed;
}
}  // namespace
