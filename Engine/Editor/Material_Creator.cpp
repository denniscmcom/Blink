// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Material_Creator.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Helpers.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Resource/Material.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <format>

void
blk::draw_material_creator(Editor_Context& context)
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	context.material_creator_size.x = viewport->WorkSize.x - context.scene_graph_size.x - context.settings_size.x;
	context.material_creator_size.y = viewport->WorkSize.y - context.status_bar_size.y - context.console_size.y;

	context.material_creator_position.x = viewport->WorkPos.x + context.scene_graph_size.x;
	context.material_creator_position.y = viewport->WorkPos.y;

	ImGui::SetNextWindowPos(to_imvec2(context.material_creator_position), ImGuiCond_Always);
	ImGui::SetNextWindowSize(to_imvec2(context.material_creator_size), ImGuiCond_Always);

	ImGui::Begin(
		"Material creator",
		&context.show_material_creator,
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
	);

	static std::string material_stem = "";
	ImGui::InputText("Material name", &material_stem);

	static std::string diffuse_map_stem = "";
	ImGui::InputText("Diffuse map", &diffuse_map_stem);

	static std::string specular_map_stem = "";
	ImGui::InputText("Specular map", &specular_map_stem);

	static float shininess = 32.0f;
	ImGui::DragFloat("Shininess", &shininess, 1.00f, 1.0f, 256.0f);

	if (ImGui::Button("Create"))
	{
		const std::string path = std::format("{}/Materials/{}.bmaterial", BLK_SOURCE_ASSETS_DIRECTORY, material_stem);
		File* file = open_file(path.c_str(), File_Access_Mode::WRITE);

		if (!file)
		{
			BLK_ERROR("Failed to create material file: %s\n", path.c_str());

			return;
		}

		Serial serial(1'024);

		serial.write(BLINK_MAGIC);
		serial.write(MATERIAL_MAGIC);
		serial.write(MATERIAL_VERSION);

		serial.write(get_resource_hash(diffuse_map_stem.c_str(), Resource_Type::TEXTURE));
		serial.write(get_resource_hash(specular_map_stem.c_str(), Resource_Type::TEXTURE));
		serial.write(shininess);

		write_file(file, serial.buffer(), serial.position());
		close_file(file);

		BLK_INFO("Material created: %s\n", path.c_str());
	}

	ImGui::End();
}
