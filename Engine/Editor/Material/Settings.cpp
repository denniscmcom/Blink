// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Material/Settings.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Resource/Material.hpp"
#include "Engine/Scene/Node.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

void
blk::draw_material_settings(Editor_Context& context)
{
	// Get ImGui viewport.

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// This widget is located at the right side of the screen, below the menu bar and above the status bar.
	// We compute its size, and position.

	context.material_context.material_settings_size.x = context.right_column_width;
	context.material_context.material_settings_size.y =
		viewport->WorkSize.y - context.viewport_context.status_bar_size.y - context.toolbar_size.y;

	context.material_context.material_settings_position.x =
		viewport->WorkPos.x + viewport->WorkSize.x - context.material_context.material_settings_size.x;
	context.material_context.material_settings_position.y = viewport->WorkPos.y + context.toolbar_size.y;

	// Pass size and position data to ImGui.

	ImGui::SetNextWindowPos(context.material_context.material_settings_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.material_context.material_settings_size, ImGuiCond_Always);

	ImGui::Begin("Material settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	// TODO (Bug): Not implemented. It should display and edit the material on
	// `Editor_Material_Context::material_prop_handle`.

	ImGui::End();
}
