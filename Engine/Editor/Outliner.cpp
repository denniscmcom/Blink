#include "Editor/Outliner.hpp"

#include "Editor/Context.hpp"
#include "Editor/Helpers.hpp"
#include "Resource/Material.hpp"
#include "Resource/Mesh.hpp"
#include "Scene/Scene_Graph.hpp"
#include "Scene/Transform.hpp"
#include "World/World.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace
{
void draw_node(blk::Editor_Context& context, blk::Pool_Handle<blk::Node> handle);
}  // namespace

void
blk::draw_outliner(Editor_Context& context)
{
	BLK_CHECK(context.world);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// ============================================================================
	// Scene graph.
	// ============================================================================

	const float available_height = viewport->WorkSize.y - context.status_bar_size.y;

	context.scene_graph_position = to_rect2(viewport->WorkPos);
	context.scene_graph_size = Rect{.x = context.left_column_width, .y = available_height * 0.6f};
	context.scene_graph_min_size = Rect{.x = LEFT_COLUMN_MIN_WIDTH, .y = context.scene_graph_size.y};
	context.scene_graph_max_size = Rect{.x = LEFT_COLUMN_MAX_WIDTH, .y = context.scene_graph_size.y};

	ImGui::SetNextWindowPos(to_imvec2(context.scene_graph_position), ImGuiCond_Always);
	ImGui::SetNextWindowSize(to_imvec2(context.scene_graph_size), ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(
		to_imvec2(context.scene_graph_min_size),
		to_imvec2(context.scene_graph_max_size)
	);

	ImGui::Begin("Scene graph", nullptr, ImGuiWindowFlags_NoMove);
	context.left_column_width = ImGui::GetWindowWidth();
	draw_node(context, context.world->scene_graph.root);
	ImGui::End();

	// ============================================================================
	// Settings.
	// ============================================================================

	context.settings_size = Rect{.x = context.right_column_width, .y = available_height};
	context.settings_min_size = Rect{.x = RIGHT_COLUMN_MIN_WIDTH, .y = context.settings_size.y};
	context.settings_max_size = Rect{.x = RIGHT_COLUMN_MAX_WIDTH, .y = context.settings_size.y};

	context.settings_position.x = viewport->WorkPos.x + viewport->WorkSize.x - context.settings_size.x;
	context.settings_position.y = viewport->WorkPos.y;

	ImGui::SetNextWindowPos(to_imvec2(context.settings_position), ImGuiCond_Always);
	ImGui::SetNextWindowSize(to_imvec2(context.settings_size), ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(to_imvec2(context.settings_min_size), to_imvec2(context.settings_max_size));

	ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove);
	context.right_column_width = ImGui::GetWindowWidth();

	if (ImGui::BeginTabBar("##Settings"))
	{
		if (ImGui::BeginTabItem("Node settings"))
		{
			if (Node* node = context.world->scene_graph.nodes.get(context.selected_node_handle))
			{
				// ============================================================================
				// Name.
				// ============================================================================

				if (ImGui::InputText("Name", &node->name, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					const std::string unique_node_name =
						make_unique_node_name(context.world->scene_graph, node->name.c_str());
					rename_node(context.world->scene_graph, context.selected_node_handle, unique_node_name.c_str());
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
						ImGui::SeparatorText("Mesh");
						const Mesh* mesh = get_mesh(node->mesh_instance.value().mesh_handle);
						std::string mesh_stem = mesh->metadata.stem;

						if (ImGui::InputText("Mesh stem", &mesh_stem, ImGuiInputTextFlags_EnterReturnsTrue))
						{
							node->mesh_instance.value().mesh_handle = load_mesh(mesh_stem.c_str());
							mesh = get_mesh(node->mesh_instance.value().mesh_handle);
						}

						ImGui::Text("Vertex count: %llu", mesh->vertices.size());
						ImGui::Text("Index count: %llu", mesh->indices.size());
					}

					if (node->mesh_instance.value().material_handle != POOL_HANDLE_NONE<Material>)
					{
						ImGui::SeparatorText("Material");
						Material* material = get_material(node->mesh_instance.value().material_handle);

						if (material->diffuse_map != POOL_HANDLE_NONE<Texture>)
						{
							const Texture* diffuse_map = get_texture(material->diffuse_map);
							std::string diffuse_map_stem = diffuse_map->metadata.stem;

							if (ImGui::InputText(
									"Diffuse map stem",
									&diffuse_map_stem,
									ImGuiInputTextFlags_EnterReturnsTrue
								))
							{
								// TODO: Pending (should I remove material as a resource and treat it Material_Instance
								// or something?
								node->mesh_instance.value().material_handle = load_material("");
							}
						}

						if (material->specular_map != POOL_HANDLE_NONE<Texture>)
						{
							const Texture* specular_map = get_texture(material->specular_map);
							std::string specular_map_stem = specular_map->metadata.stem;

							if (ImGui::InputText(
									"Specular map stem",
									&specular_map_stem,
									ImGuiInputTextFlags_EnterReturnsTrue
								))
							{
							}
						}

						// FIXME: Currently this is not being modified on GPU (only CPU). I need a `dirty` flag to
						// reupload
						//  the changes to the GPU or something.
						ImGui::DragFloat("Shininess", &material->shininess, 0.01f);
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
	BLK_CHECK(context.world);

	const blk::Node* node = context.world->scene_graph.nodes.get(handle);

	if (!node)
	{
		return;
	}

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

	if (handle == context.selected_node_handle)
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
		context.selected_node_handle = handle;
	}

	if (is_open)
	{
		blk::Pool_Handle<blk::Node> child_handle = node->first_child_handle;

		while (const blk::Node* child = context.world->scene_graph.nodes.get(child_handle))
		{
			draw_node(context, child_handle);
			child_handle = child->next_sibling_handle;
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}
}  // namespace
