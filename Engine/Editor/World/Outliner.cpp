// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/World/Outliner.hpp"

#include "Engine/Core/Math/Unit.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Resource/Material.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Resource/Texture.hpp"
#include "Engine/Scene/Light.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/Scene/Scene_Graph.hpp"
#include "Engine/World/World.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace
{
void draw_node(blk::Editor_Context& context, blk::Pool_Handle<blk::Node> handle);
}  // namespace

void
blk::draw_outliner(Editor_Context& context)
{
	BLK_CHECK(context.world_context._game_world);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// ============================================================================
	// Scene graph.
	// ============================================================================

	const float available_height =
		viewport->WorkSize.y - context.viewport_context.status_bar_size.y - context.toolbar_size.y;

	context.world_context.scene_graph_position.x = viewport->WorkPos.x;
	context.world_context.scene_graph_position.y = viewport->WorkPos.y + context.toolbar_size.y;
	context.world_context.scene_graph_size = ImVec2(context.left_column_width, available_height * 0.6f);
	context.world_context.scene_graph_min_size = ImVec2(LEFT_COLUMN_MIN_WIDTH, context.world_context.scene_graph_size.y);
	context.world_context.scene_graph_max_size = ImVec2(LEFT_COLUMN_MAX_WIDTH, context.world_context.scene_graph_size.y);

	ImGui::SetNextWindowPos(context.world_context.scene_graph_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.world_context.scene_graph_size, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(
		context.world_context.scene_graph_min_size,
		context.world_context.scene_graph_max_size
	);

	ImGui::Begin("Scene graph", nullptr, ImGuiWindowFlags_NoMove);
	context.left_column_width = ImGui::GetWindowWidth();
	draw_node(context, context.world_context._game_world->scene_graph.root);
	ImGui::End();

	// ============================================================================
	// Settings.
	// ============================================================================

	context.world_context.settings_size = ImVec2(context.right_column_width, available_height);
	context.world_context.settings_min_size = ImVec2(RIGHT_COLUMN_MIN_WIDTH, context.world_context.settings_size.y);
	context.world_context.settings_max_size = ImVec2(RIGHT_COLUMN_MAX_WIDTH, context.world_context.settings_size.y);

	context.world_context.settings_position.x =
		viewport->WorkPos.x + viewport->WorkSize.x - context.world_context.settings_size.x;
	context.world_context.settings_position.y = viewport->WorkPos.y + context.toolbar_size.y;

	ImGui::SetNextWindowPos(context.world_context.settings_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.world_context.settings_size, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(context.world_context.settings_min_size, context.world_context.settings_max_size);

	ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove);
	context.right_column_width = ImGui::GetWindowWidth();

	if (ImGui::BeginTabBar("##Settings"))
	{
		if (ImGui::BeginTabItem("Node settings"))
		{
			if (Node* node =
					context.world_context._game_world->scene_graph.nodes.get(context.world_context.selected_node_handle))
			{
				// ============================================================================
				// Name.
				// ============================================================================

				if (ImGui::InputText("Name", &node->name, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					const std::string unique_node_name =
						make_unique_node_name(context.world_context._game_world->scene_graph, node->name.c_str());
					rename_node(
						context.world_context._game_world->scene_graph,
						context.world_context.selected_node_handle,
						unique_node_name.c_str()
					);
				}

				// ============================================================================
				// Transform.
				// ============================================================================

				if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::DragFloat3("Position", &node->transform.position.x, 0.01f);

					Vector3 rotation = {};
					rotation.x = to_degrees(Radians{node->transform.rotation.x}).value;
					rotation.y = to_degrees(Radians{node->transform.rotation.y}).value;
					rotation.z = to_degrees(Radians{node->transform.rotation.z}).value;

					if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f))
					{
						node->transform.rotation.x = to_radians(Degrees{rotation.x}).value;
						node->transform.rotation.y = to_radians(Degrees{rotation.y}).value;
						node->transform.rotation.z = to_radians(Degrees{rotation.z}).value;
					}

					ImGui::DragFloat3("Scale", &node->transform.scale.x, 0.01f);

					ImGui::TreePop();
				}

				// ============================================================================
				// Mesh instance.
				// ============================================================================

				if (node->mesh_instance && ImGui::TreeNodeEx("Mesh instance", ImGuiTreeNodeFlags_DefaultOpen))
				{
					if (node->mesh_instance.value().mesh_handle != POOL_HANDLE_NONE<Mesh>)
					{
						const Mesh* mesh = get_mesh(node->mesh_instance.value().mesh_handle);
						std::string mesh_stem = "";

						if (mesh)
						{
							mesh_stem = mesh->metadata.stem;
						}

						if (ImGui::InputText("Mesh", &mesh_stem, ImGuiInputTextFlags_EnterReturnsTrue))
						{
							node->mesh_instance.value().mesh_handle = load_mesh(mesh_stem.c_str());
						}
					}

					if (node->mesh_instance.value().material_handle != POOL_HANDLE_NONE<Material>)
					{
						const Material* material = get_material(node->mesh_instance.value().material_handle);
						std::string material_stem = "";

						if (material)
						{
							material_stem = material->metadata.stem;
						}

						if (ImGui::InputText("Material", &material_stem, ImGuiInputTextFlags_EnterReturnsTrue))
						{
							node->mesh_instance.value().material_handle = load_material(material_stem.c_str());
						}
					}

					ImGui::TreePop();
				}

				// ============================================================================
				// Point light.
				// ============================================================================

				if (node->point_light && ImGui::TreeNodeEx("Point light", ImGuiTreeNodeFlags_DefaultOpen))
				{
					Point_Light& point_light = node->point_light.value();
					ImGui::ColorPicker3("Color", &point_light.color.r);

					ImGui::TreePop();
				}
			}
			else
			{
				ImGui::Text("No node selected");
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("World settings"))
		{
			ImGui::EndTabItem();
		}
	}

	ImGui::EndTabBar();
	ImGui::End();
}

namespace
{
void
draw_node(blk::Editor_Context& context, const blk::Pool_Handle<blk::Node> handle)
{
	BLK_CHECK(context.world_context._game_world);

	const blk::Node* node = context.world_context._game_world->scene_graph.nodes.get(handle);

	if (!node)
	{
		return;
	}

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

	if (handle == context.world_context.selected_node_handle)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	if (node->first_child_handle == blk::POOL_HANDLE_NONE<blk::Node>)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	// Node names are not unique, so the handle provides the ImGui id instead of the label.
	const size_t node_hash = blk::Pool_Handle_Hash<blk::Node>{}(handle);
	ImGui::PushID(static_cast<int>(node_hash));

	const bool is_open = ImGui::TreeNodeEx(node->name.empty() ? "Unnamed" : node->name.c_str(), flags);

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		context.world_context.selected_node_handle = handle;
	}

	if (is_open)
	{
		blk::Pool_Handle<blk::Node> child_handle = node->first_child_handle;

		while (const blk::Node* child = context.world_context._game_world->scene_graph.nodes.get(child_handle))
		{
			draw_node(context, child_handle);
			child_handle = child->next_sibling_handle;
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}
}  // namespace
