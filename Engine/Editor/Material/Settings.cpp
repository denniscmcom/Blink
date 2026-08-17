// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Material/Settings.hpp"

#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Resource/Material.hpp"
#include "Engine/Resource/Resource.hpp"
#include "Engine/Scene/Node.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <format>

void
blk::draw_material_settings(Editor_Context& context)
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	context.material_context.material_settings_size.x = context.right_column_width;
	context.material_context.material_settings_size.y =
		viewport->WorkSize.y - context.viewport_context.status_bar_size.y - context.toolbar_size.y;

	context.material_context.material_settings_position.x =
		viewport->WorkSize.x - context.material_context.material_settings_size.x;
	context.material_context.material_settings_position.y = viewport->WorkPos.y + context.toolbar_size.y;

	ImGui::SetNextWindowPos(context.material_context.material_settings_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.material_context.material_settings_size, ImGuiCond_Always);

	ImGui::Begin("Material settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	Node* material_node =
		get_entity_node(context.material_context.world, context.material_context.material_prop_handle);

	if (!material_node)
	{
		BLK_ERROR("Failed to find material prop node\n");

		return;
	}

	static std::string albedo_stem = "";
	static std::string normal_stem = "";
	static std::string orm_stem = "";

	if (material_node->mesh_instance)
	{
		if (Material* material = get_material(material_node->mesh_instance.value().material_handle))
		{
			if (Texture* albedo = get_texture(material->albedo))
			{
				albedo_stem = albedo->metadata.stem;
			}

			if (Texture* normal = get_texture(material->normal))
			{
				normal_stem = normal->metadata.stem;
			}

			if (Texture* orm = get_texture(material->orm))
			{
				orm_stem = orm->metadata.stem;
			}
		}

		static std::string material_name = "";

		ImGui::InputText("Name", &material_name);
		ImGui::InputText("Albedo", &albedo_stem);
		ImGui::InputText("Normal", &normal_stem);
		ImGui::InputText("ORM", &orm_stem);

		if (ImGui::Button("Create"))
		{
			Serial serial(1'024);

			serial.write(BLINK_MAGIC);
			serial.write(MATERIAL_MAGIC);
			serial.write(MATERIAL_VERSION);

			// FIXME: Check first if these files exist.
			serial.write(get_resource_hash(albedo_stem.c_str(), Resource_Type::TEXTURE));
			serial.write(get_resource_hash(normal_stem.c_str(), Resource_Type::TEXTURE));
			serial.write(get_resource_hash(orm_stem.c_str(), Resource_Type::TEXTURE));

			const std::string material_path =
				std::format("{}/Materials/{}.bmaterial", BLK_SOURCE_ASSETS_DIRECTORY, material_name);

			if (File* material_file = open_file(material_path.c_str(), File_Access_Mode::WRITE))
			{
				write_file(material_file, serial.buffer(), serial.position());
				close_file(material_file);
				BLK_INFO("Material created: %s\n", material_path.c_str());
			}
			else
			{
				BLK_ERROR("Failed to open material file to write: %s\n", material_path.c_str());
			}
		}
	}

	ImGui::End();
}
