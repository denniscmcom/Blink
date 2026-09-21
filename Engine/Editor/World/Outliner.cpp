// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/World/Outliner.hpp"

#include "Engine/Core/Math/Unit.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Core/String.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Resource/Material.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Scene/Graph.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/Scene/Point_Light.hpp"
#include "Engine/World/World.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace
{
/// Helper function to draw the node tree starting at `handle`.
void draw_node(blk::Editor_Context& context, blk::Pool_Handle<blk::Node> handle);
}  // namespace

void
blk::draw_outliner(Editor_Context& context)
{
	if (!BLK_VERIFY(context.world_context.game_world))
	{
		return;
	}

	// Get ImGui viewport.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Compute size and position for the outliner.
	// The outliner is located at the left side of the screen, below the menu bar and above the status bar.

	const float available_height =
		viewport->WorkSize.y - context.viewport_context.status_bar_size.y - context.toolbar_size.y;

	context.world_context.scene_graph_position.x = viewport->WorkPos.x;
	context.world_context.scene_graph_position.y = viewport->WorkPos.y + context.toolbar_size.y;
	context.world_context.scene_graph_size = ImVec2(context.left_column_width, available_height * 0.6f);
	context.world_context.scene_graph_min_size =
		ImVec2(LEFT_COLUMN_MIN_WIDTH, context.world_context.scene_graph_size.y);
	context.world_context.scene_graph_max_size =
		ImVec2(LEFT_COLUMN_MAX_WIDTH, context.world_context.scene_graph_size.y);

	// Pass size, position, and constraints to ImGui.

	ImGui::SetNextWindowPos(context.world_context.scene_graph_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.world_context.scene_graph_size, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(
		context.world_context.scene_graph_min_size,
		context.world_context.scene_graph_max_size
	);

	// We draw first the scene graph. The scene graph is resizable horizontally by the user within limits.

	ImGui::Begin("Scene graph", nullptr, ImGuiWindowFlags_NoMove);

	// Ask ImGui for the width of the scene graph widget.
	context.left_column_width = ImGui::GetWindowWidth();

	// We draw nodes recursively, starting at the root.
	draw_node(context, context.world_context.game_world->scene_graph.root);

	ImGui::End();

	// Compute size and position of the node settings widget.
	// This widget is located at the right of the screen, below the menu and above the status bar.

	context.world_context.settings_size = ImVec2(context.right_column_width, available_height);
	context.world_context.settings_min_size = ImVec2(RIGHT_COLUMN_MIN_WIDTH, context.world_context.settings_size.y);
	context.world_context.settings_max_size = ImVec2(RIGHT_COLUMN_MAX_WIDTH, context.world_context.settings_size.y);

	context.world_context.settings_position.x =
		viewport->WorkPos.x + viewport->WorkSize.x - context.world_context.settings_size.x;
	context.world_context.settings_position.y = viewport->WorkPos.y + context.toolbar_size.y;

	// Pass size and position to ImGui.

	ImGui::SetNextWindowPos(context.world_context.settings_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.world_context.settings_size, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(
		context.world_context.settings_min_size,
		context.world_context.settings_max_size
	);

	// Draw the settings widget.

	ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove);

	// Ask ImGui for the width of the settings widget.
	context.right_column_width = ImGui::GetWindowWidth();

	// Inside the settings widget, we have two tabs: one for node settings and another for world settings.

	if (ImGui::BeginTabBar("##Settings"))
	{
		// Node settings tab.

		if (ImGui::BeginTabItem("Node settings"))
		{
			// Get the current selected node.

			if (Node* node =
					get_node(context.world_context.game_world->scene_graph, context.world_context.selected_node_handle))
			{
				// Display node name.

				// We copy the node name to this local variable to avoid modifying the actual node name on every
				// keystroke.
				char node_name[MAX_NODE_NAME_SIZE] = {};

				BLK_IF_NOT_SNPRINTF(node->name, node_name, MAX_NODE_NAME_SIZE, "%s")
				{
					BLK_ERROR("Failed to copy node name to local variable\n");
				}

				if (ImGui::InputText("Name", node_name, MAX_NODE_NAME_SIZE, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					BLK_IF_NOT_SUCCESS(rename_node(
						context.world_context.game_world->scene_graph,
						context.world_context.selected_node_handle,
						node_name
					))
					{
						BLK_ERROR("Failed to rename node\n");
					}
				}

				// Display node transform.

				if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
				{
					// Display node position.
					ImGui::DragFloat3("Position", &node->transform.position.x, 0.01f);

					// We display rotation in degrees, so we have to convert it since we store it in radians.
					Vector3 rotation = {};
					rotation.x = to_degrees(Radians{node->transform.rotation.x}).value;
					rotation.y = to_degrees(Radians{node->transform.rotation.y}).value;
					rotation.z = to_degrees(Radians{node->transform.rotation.z}).value;

					// Display node rotation.

					if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f))
					{
						node->transform.rotation.x = to_radians(Degrees{rotation.x}).value;
						node->transform.rotation.y = to_radians(Degrees{rotation.y}).value;
						node->transform.rotation.z = to_radians(Degrees{rotation.z}).value;
					}

					// Display node scale.

					ImGui::DragFloat3("Scale", &node->transform.scale.x, 0.01f);

					ImGui::TreePop();
				}

				// Depending on the `Node::type` we display either the mesh instance or point light data.

				switch (node->type)
				{
				case Node_Type::MESH_INSTANCE: {
					// Display node instance.

					if (ImGui::TreeNodeEx("Mesh instance", ImGuiTreeNodeFlags_DefaultOpen))
					{
						// We display the stem of each mesh-material pair in the mesh instance.
						// We Loop over its capacity to display available slots, so they can be modified.

						for (size_t slot_index = 0; slot_index < node->mesh_instance.mesh_handles.count; ++slot_index)
						{
							ImGui::Text("Slot %llu", slot_index);

							const Pool_Handle<Mesh> mesh_handle = node->mesh_instance.mesh_handles.buffer[slot_index];
							Mesh* mesh = get_mesh(mesh_handle);

							// Show mesh stem.
							char mesh_stem[MAX_RESOURCE_STEM_SIZE] = {};

							if (mesh)
							{
								// Copy mesh stem.

								BLK_IF_NOT_SNPRINTF(mesh->metadata.stem, mesh_stem, MAX_RESOURCE_STEM_SIZE, "%s")
								{
									BLK_ERROR("Failed to copy mesh stem to local variable\n");
								}
							}

							if (ImGui::InputText(
									"Mesh stem",
									mesh_stem,
									MAX_NODE_NAME_SIZE,
									ImGuiInputTextFlags_EnterReturnsTrue
								))
							{
								// Load new mesh.

								const Pool_Handle<Mesh> new_mesh_handle = load_mesh(mesh_stem);

								BLK_IF_NOT_SUCCESS(set(node->mesh_instance.mesh_handles, new_mesh_handle, slot_index))
								{
									BLK_ERROR("Failed to set mesh handle in slot %zu\n", slot_index);
								}
							}

							const Pool_Handle<Material> material_handle =
								node->mesh_instance.material_handles.buffer[slot_index];
							Material* material = get_material(material_handle);

							// Show material stem.
							char material_stem[MAX_RESOURCE_STEM_SIZE] = {};

							if (material)
							{
								// Copy material stem.

								BLK_IF_NOT_SNPRINTF(
									material->metadata.stem,
									material_stem,
									MAX_RESOURCE_STEM_SIZE,
									"%s"
								)
								{
									BLK_ERROR("Failed to copy material stem to local variable\n");
								}
							}

							if (ImGui::InputText(
									"Material",
									material_stem,
									MAX_RESOURCE_STEM_SIZE,
									ImGuiInputTextFlags_EnterReturnsTrue
								))
							{
								// Load new material.

								const Pool_Handle<Material> new_material_handle = load_material(material_stem);

								BLK_IF_NOT_SUCCESS(
									set(node->mesh_instance.material_handles, new_material_handle, slot_index)
								)
								{
									BLK_ERROR("Failed to set material handle in slot %zu\n", slot_index);
								}
							}
						}

						ImGui::Separator();

						ImGui::TreePop();
					}
				}
				break;
				case Node_Type::POINT_LIGHT: {
					// Display point light data.

					if (ImGui::TreeNodeEx("Point light", ImGuiTreeNodeFlags_DefaultOpen))
					{
						// This relies on `Color_RGB` being packed together.
						ImGui::ColorPicker3("Color", &node->point_light.color.r);

						ImGui::TreePop();
					}
				}
				break;
				case Node_Type::SPATIAL: {
					// Nothing special to a `SPATIAL` node.
				}
				break;
				case Node_Type::DIRECTIONAL_LIGHT: {
					// Display directional light data.

					if (ImGui::TreeNodeEx("Directional light", ImGuiTreeNodeFlags_DefaultOpen))
					{
						// This relies on `Color_RGB` being packed together.
						ImGui::ColorPicker3("Color", &node->directional_light.color.r);
						ImGui::Checkbox("Is sun", &node->directional_light.is_sun);

						ImGui::TreePop();
					}
				}
				break;
				}
			}
			else
			{
				ImGui::Text("No node selected");
			}

			ImGui::EndTabItem();
		}

		// World settings tab.

		if (ImGui::BeginTabItem("World settings"))
		{

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

namespace
{
void
draw_node(blk::Editor_Context& context, const blk::Pool_Handle<blk::Node> handle)
{
	if (!BLK_VERIFY(context.world_context.game_world))
	{
		return;
	}

	// Get the node to draw.

	const blk::Node* node = get_node(context.world_context.game_world->scene_graph, handle);

	if (!node)
	{
		BLK_ERROR("Failed to find node to draw\n");

		return;
	}

	// Tree node flags.

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

	// If the node to draw is the one currently selected in the outliner, we add the selected flag.

	if (handle == context.world_context.selected_node_handle)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	// If the node to draw does not have children, this node is a leaf.

	if (node->first_child_handle == blk::POOL_HANDLE_NONE<blk::Node>)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	// Node names are unique, so we use it as ImGui ID.
	ImGui::PushID(node->name);

	const bool is_open = ImGui::TreeNodeEx(node->name, flags);

	// Handle node selection.

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		context.world_context.selected_node_handle = handle;
	}

	// If the node tree is open, we draw its subtree.

	if (is_open)
	{
		blk::Pool_Handle<blk::Node> child_handle = node->first_child_handle;

		while (const blk::Node* child = get_node(context.world_context.game_world->scene_graph, child_handle))
		{
			draw_node(context, child_handle);
			child_handle = child->next_sibling_handle;
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}
}  // namespace
