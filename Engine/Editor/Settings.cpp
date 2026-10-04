// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Settings.hpp"

#include "Engine/Core/String.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/Material.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Resource/Texture.hpp"
#include "Engine/Scene/Node.hpp"

#include <imgui.h>
#include <stddef.h>
#include <stdint.h>

namespace
{
/// Draws `label` on its own line and makes the next widget fill the available width below it.
/// The widget should use a hidden label (`##name`) since this one already shows it.
void draw_property_label(const char* label);
}  // namespace

void
blk::draw_settings(Editor_Context& context)
{
	// Inside the settings widget, we have two tabs: one for node settings and another for world settings.

	ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove);

	// Ask ImGui for the width of the settings widget.
	context.right_column_width = ImGui::GetWindowWidth();
	if (ImGui::BeginTabBar("##Settings"))
	{
		// Node settings tab.

		if (ImGui::BeginTabItem("Node"))
		{
			// Get the current selected node.

			if (Node* node = get_node(context.active_world->scene_graph, context.selected_node_handle))
			{
				// Display node name.

				// We copy the node name to this local variable to avoid modifying the actual node name on every
				// keystroke.
				char node_name[MAX_NODE_NAME_SIZE] = {};

				BLK_IF_NOT_SNPRINTF(node_name, MAX_NODE_NAME_SIZE, "%s", node->name)
				{
					BLK_ERROR("Failed to copy node name to local variable\n");
				}

				draw_property_label("Name");
				if (ImGui::InputText("##Name", node_name, MAX_NODE_NAME_SIZE, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					BLK_IF_NOT_SUCCESS(
						rename_node(context.active_world->scene_graph, context.selected_node_handle, node_name)
					)
					{
						BLK_ERROR("Failed to rename node\n");
					}
				}

				// Display node transform.

				if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
				{
					// Display node position.
					draw_property_label("Position");
					ImGui::DragFloat3("##Position", &node->transform.position.x, 0.01f);

					// We display rotation in degrees, so we have to convert it since we store it in radians.
					Vector3 rotation = {};
					rotation.x = to_degrees(Radians{node->transform.rotation.x}).value;
					rotation.y = to_degrees(Radians{node->transform.rotation.y}).value;
					rotation.z = to_degrees(Radians{node->transform.rotation.z}).value;

					// Display node rotation.

					draw_property_label("Rotation");
					if (ImGui::DragFloat3("##Rotation", &rotation.x, 0.1f))
					{
						node->transform.rotation.x = to_radians(Degrees{rotation.x}).value;
						node->transform.rotation.y = to_radians(Degrees{rotation.y}).value;
						node->transform.rotation.z = to_radians(Degrees{rotation.z}).value;
					}

					// Display node scale.

					draw_property_label("Scale");
					ImGui::DragFloat3("##Scale", &node->transform.scale.x, 0.01f);

					ImGui::TreePop();
				}

				// Depending on the `Node::type` we display either the mesh instance or point light data.

				switch (node->type)
				{
				case Node_Type::MESH_REF: {
					if (ImGui::TreeNodeEx("Mesh reference", ImGuiTreeNodeFlags_DefaultOpen))
					{
						// Display mesh stem.

						Mesh* mesh = get_mesh(node->mesh_ref.mesh_handle);

						// Show mesh stem.
						char mesh_stem[MAX_RESOURCE_STEM_SIZE] = {};

						if (mesh)
						{
							// Copy mesh stem.

							BLK_IF_NOT_SNPRINTF(mesh_stem, MAX_RESOURCE_STEM_SIZE, "%s", mesh->metadata.stem)
							{
								BLK_ERROR("Failed to copy mesh stem to local variable\n");
							}
						}

						draw_property_label("Mesh stem");
						if (ImGui::InputText(
								"##Mesh stem",
								mesh_stem,
								MAX_RESOURCE_STEM_SIZE,
								ImGuiInputTextFlags_EnterReturnsTrue
							))
						{
							// Load new mesh. On failure we keep the current one.
							if (const Pool_Handle<Mesh> mesh_handle = load_mesh(mesh_stem);
								mesh_handle != POOL_HANDLE_NONE<Mesh>)
							{
								node->mesh_ref.mesh_handle = mesh_handle;
							}
							else
							{
								BLK_ERROR("Failed to load the mesh `%s`\n", mesh_stem);
							}
						}

						// Display material stem.

						Material* material = get_material(node->mesh_ref.material_handle);

						// Show material stem.
						char material_stem[MAX_RESOURCE_STEM_SIZE] = {};

						if (material)
						{
							// Copy material stem.

							BLK_IF_NOT_SNPRINTF(material_stem, MAX_RESOURCE_STEM_SIZE, "%s", material->metadata.stem)
							{
								BLK_ERROR("Failed to copy material stem to local variable\n");
							}
						}

						draw_property_label("Material stem");
						if (ImGui::InputText(
								"##Material stem",
								material_stem,
								MAX_RESOURCE_STEM_SIZE,
								ImGuiInputTextFlags_EnterReturnsTrue
							))
						{
							// Load new material. On failure we keep the current one.
							if (const Pool_Handle<Material> material_handle = load_material(material_stem);
								material_handle != POOL_HANDLE_NONE<Material>)
							{
								node->mesh_ref.material_handle = material_handle;
							}
							else
							{
								BLK_ERROR("Failed to load the material `%s`\n", material_stem);
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

						draw_property_label("Color");
						ImGui::ColorEdit3("##Color", &node->point_light.color.r, ImGuiColorEditFlags_Float);

						ImGui::TreePop();
					}
				}
				break;
				case Node_Type::SPATIAL: {
					// Nothing special to a `SPATIAL` node.
				}
				break;
				case Node_Type::TERRAIN: {
					// Display terrain data.

					if (ImGui::TreeNodeEx("Terrain", ImGuiTreeNodeFlags_DefaultOpen))
					{
						// Read-only data.
						ImGui::BeginDisabled();

						draw_property_label("UV offset");
						ImGui::DragFloat2("##UV offset", &node->terrain_ref.uv_offset.x, 0.01f);

						draw_property_label("UV scale");
						ImGui::DragFloat("##UV scale", &node->terrain_ref.uv_scale, 0.01f);

						draw_property_label("LOD");
						ImGui::DragScalar("##LOD", ImGuiDataType_U32, &node->terrain_ref.lod);

						ImGui::EndDisabled();

						ImGui::TreePop();
					}
				}
				break;
				case Node_Type::DIRECTIONAL_LIGHT: {
					// Display directional light data.

					if (ImGui::TreeNodeEx("Directional light", ImGuiTreeNodeFlags_DefaultOpen))
					{
						// This relies on `Color_RGB` being packed together.

						draw_property_label("Color");
						ImGui::ColorEdit3("##Color", &node->directional_light.color.r, ImGuiColorEditFlags_Float);

						draw_property_label("Intensity");
						ImGui::DragFloat(
							"##Intensity",
							&node->directional_light.intensity,
							0.1f,
							0.0f,
							1000.0f,
							"%.2f",
							ImGuiSliderFlags_AlwaysClamp
						);

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

		if (ImGui::BeginTabItem("World"))
		{
			// Atmosphere.

			Atmosphere_Settings& atmosphere = context.active_world->settings.atmosphere;

			if (ImGui::TreeNodeEx("Atmosphere", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// Sun node, entered by name.

				Scene_Graph& scene_graph = context.active_world->scene_graph;

				// We copy the current sun name to this local variable, as for the node name, so the node is only looked
				// up once the user presses enter. It stays empty if there is no sun or its node was despawned.
				char sun_name[MAX_NODE_NAME_SIZE] = {};

				if (const Node* sun_node = get_node(scene_graph, atmosphere.sun_node_handle))
				{
					BLK_IF_NOT_SNPRINTF(sun_name, MAX_NODE_NAME_SIZE, "%s", sun_node->name)
					{
						BLK_ERROR("Failed to copy sun node name to local variable\n");
					}
				}

				draw_property_label("Sun node name");

				if (ImGui::InputText("##Sun", sun_name, MAX_NODE_NAME_SIZE, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					// An empty name removes the sun.

					if (sun_name[0] == '\0')
					{
						atmosphere.sun_node_handle = POOL_HANDLE_NONE<Node>;
					}
					else
					{
						// TODO (Feature): not implemented.
						// Implement a picker to pick a directional light from the scene graph.
						BLK_NOT_IMPLEMENTED();
					}
				}

				// Skybox passes. A checkbox is narrow, so its label fits beside it.

				if (ImGui::TreeNodeEx("Passes", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Checkbox("Transmittance", &atmosphere.enable_transmittance);
					ImGui::Checkbox("Multiscattering", &atmosphere.enable_multiscattering);
					ImGui::Checkbox("Sky-view", &atmosphere.enable_sky_view);
					ImGui::Checkbox("Aerial", &atmosphere.enable_aerial);

					ImGui::TreePop();
				}

				// Participating media. Coefficients are a few thousandths per kilometer, so they need a fine drag
				// speed and more decimals than the default format shows.

				if (ImGui::TreeNodeEx("Medium", ImGuiTreeNodeFlags_DefaultOpen))
				{
					draw_property_label("Rayleigh scattering");
					ImGui::DragFloat3(
						"##Rayleigh scattering",
						&atmosphere.rayleigh_scattering.x,
						0.0001f,
						0.0f,
						1.0f,
						"%.6f",
						ImGuiSliderFlags_AlwaysClamp
					);

					draw_property_label("Mie scattering");
					ImGui::DragFloat(
						"##Mie scattering",
						&atmosphere.mie_scattering,
						0.0001f,
						0.0f,
						1.0f,
						"%.6f",
						ImGuiSliderFlags_AlwaysClamp
					);

					draw_property_label("Mie absorption");
					ImGui::DragFloat(
						"##Mie absorption",
						&atmosphere.mie_absorption,
						0.0001f,
						0.0f,
						1.0f,
						"%.6f",
						ImGuiSliderFlags_AlwaysClamp
					);

					draw_property_label("Ozone absorption");
					ImGui::DragFloat3(
						"##Ozone absorption",
						&atmosphere.ozone_absorption.x,
						0.0001f,
						0.0f,
						1.0f,
						"%.6f",
						ImGuiSliderFlags_AlwaysClamp
					);

					// The shader divides by the scales and by the ozone low point, so none of them may reach zero.

					draw_property_label("Rayleigh scale");
					ImGui::DragFloat(
						"##Rayleigh scale",
						&atmosphere.rayleigh_scale,
						0.01f,
						0.01f,
						100.0f,
						"%.3f km",
						ImGuiSliderFlags_AlwaysClamp
					);

					draw_property_label("Mie scale");
					ImGui::DragFloat(
						"##Mie scale",
						&atmosphere.mie_scale,
						0.01f,
						0.01f,
						100.0f,
						"%.3f km",
						ImGuiSliderFlags_AlwaysClamp
					);

					draw_property_label("Ozone mid point");
					ImGui::DragFloat(
						"##Ozone mid point",
						&atmosphere.ozone_mid_point,
						0.1f,
						0.0f,
						100.0f,
						"%.3f km",
						ImGuiSliderFlags_AlwaysClamp
					);

					draw_property_label("Ozone low point");
					ImGui::DragFloat(
						"##Ozone low point",
						&atmosphere.ozone_low_point,
						0.1f,
						0.01f,
						100.0f,
						"%.3f km",
						ImGuiSliderFlags_AlwaysClamp
					);

					// The phase function divides by zero at `g = ±1`.
					draw_property_label("Mie asymmetry");
					ImGui::DragFloat(
						"##Mie asymmetry",
						&atmosphere.mie_asymmetry,
						0.001f,
						-0.999f,
						0.999f,
						"%.3f",
						ImGuiSliderFlags_AlwaysClamp
					);

					ImGui::TreePop();
				}

				// Planet geometry. Each radius is clamped against the other so the atmosphere always has a
				// thickness, which the LUT parameterizations rely on.

				if (ImGui::TreeNodeEx("Planet", ImGuiTreeNodeFlags_DefaultOpen))
				{
					draw_property_label("Planet radius");
					ImGui::DragFloat(
						"##Planet radius",
						&atmosphere.planet_radius,
						1.0f,
						1.0f,
						atmosphere.atmosphere_radius - 1.0f,
						"%.1f km",
						ImGuiSliderFlags_AlwaysClamp
					);

					draw_property_label("Atmosphere radius");
					ImGui::DragFloat(
						"##Atmosphere radius",
						&atmosphere.atmosphere_radius,
						1.0f,
						atmosphere.planet_radius + 1.0f,
						100000.0f,
						"%.1f km",
						ImGuiSliderFlags_AlwaysClamp
					);

					draw_property_label("Ground albedo");
					ImGui::ColorEdit3("##Ground albedo", &atmosphere.ground_albedo.r, ImGuiColorEditFlags_Float);

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			// Terrain.

			if (ImGui::TreeNodeEx("Terrain", ImGuiTreeNodeFlags_DefaultOpen))
			{
				Terrain_Settings& terrain = context.active_world->settings.terrain;

				// Terrain heightmap texture entered by stem.
				char heightmap_stem[MAX_RESOURCE_STEM_SIZE] = {};

				if (const Texture* heightmap = get_texture(terrain.heightmap_handle))
				{
					// Display stem of the current heighmap if it exists.

					BLK_IF_NOT_SNPRINTF(heightmap_stem, MAX_RESOURCE_STEM_SIZE, "%s", heightmap->metadata.stem)
					{
						BLK_ERROR("Failed to copy heightmap stem to local variable\n");
					}
				}

				draw_property_label("Heightmap");

				ImGui::InputText("##Heightmap", heightmap_stem, MAX_RESOURCE_STEM_SIZE);

				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					// Load the terrain heightmap.

					if (const Pool_Handle<Texture> heightmap_handle = load_texture(heightmap_stem);
						heightmap_handle != POOL_HANDLE_NONE<Texture>)
					{
						unload_texture(terrain.heightmap_handle);
						terrain.heightmap_handle = heightmap_handle;
						context.active_world->flags |= WORLD_TERRAIN_DIRTY_BIT;

						BLK_IF_NOT_SUCCESS(bake_renderer(context.active_world->settings))
						{
							BLK_ERROR("Failed to bake the renderer with the new terrain heightmap\n");
						}
					}
					else
					{
						BLK_ERROR("Failed to load the terrain heightmap `%s`\n", heightmap_stem);
					}
				}

				// Every edit rebuilds the terrain, so we only mark it dirty once the edit is finished, not on every
				// drag step.

				draw_property_label("Width");
				ImGui::DragFloat("##Width", &terrain.width, 0.01f, 0, 16'384, "%.3f", ImGuiSliderFlags_AlwaysClamp);

				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					context.active_world->flags |= WORLD_TERRAIN_DIRTY_BIT;
				}

				draw_property_label("Height");
				ImGui::DragFloat("##Height", &terrain.height, 0.01f, 0, 16'384, "%.3f", ImGuiSliderFlags_AlwaysClamp);

				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					context.active_world->flags |= WORLD_TERRAIN_DIRTY_BIT;
				}

				// The loaded chunks were split with the previous resolution and chunk texel count, so the streamer
				// loads them again when either changes.

				draw_property_label("Resolution");
				constexpr uint32_t resolution_min = 1;

				ImGui::DragScalar(
					"##Resolution",
					ImGuiDataType_U32,
					&terrain.resolution,
					0.1f,
					&resolution_min,
					nullptr,
					nullptr,
					ImGuiSliderFlags_AlwaysClamp
				);

				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					// A lower resolution can leave the chunk larger than the terrain.
					if (terrain.chunk_texel_count > terrain.resolution)
					{
						terrain.chunk_texel_count = terrain.resolution;
					}

					context.active_world->flags |= WORLD_TERRAIN_DIRTY_BIT;
				}

				// A chunk cannot be larger than the terrain, otherwise no chunk fits in it. Without a resolution there
				// is no upper bound yet, so the field stays disabled until one is set.

				draw_property_label("Chunk texel count");
				constexpr uint32_t chunk_texel_count_min = 1;
				const uint32_t chunk_texel_count_max = terrain.resolution;

				ImGui::BeginDisabled(terrain.resolution == 0);

				ImGui::DragScalar(
					"##Chunk texel count",
					ImGuiDataType_U32,
					&terrain.chunk_texel_count,
					0.1f,
					&chunk_texel_count_min,
					&chunk_texel_count_max,
					nullptr,
					ImGuiSliderFlags_AlwaysClamp
				);

				ImGui::EndDisabled();

				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					context.active_world->flags |= WORLD_TERRAIN_DIRTY_BIT;
				}

				// Dislay read-only valued.
				ImGui::BeginDisabled();

				float chunk_size = compute_chunk_size(terrain);

				draw_property_label("Chunk size");
				ImGui::DragFloat(
					"##Chunk size",
					&chunk_size,
					0.1f,
					1.0f,
					16'384.0f,
					"%.1f m",
					ImGuiSliderFlags_AlwaysClamp
				);

				ImGui::EndDisabled();

				// LOD. The streamer updates every chunk's LOD each frame, so edits need no rebuild.

				// Each radius is clamped against its neighbours so the radii stay strictly increasing.

				for (size_t i = 0; i < terrain.lod_radii.capacity; ++i)
				{
					char lod_radius_label[32] = {};

					BLK_IF_NOT_SNPRINTF(lod_radius_label, sizeof(lod_radius_label), "LOD %zu radius", i)
					{
						BLK_ERROR("Failed to format LOD radius label\n");
					}

					const int32_t lod_radius_min = i == 0 ? 0 : terrain.lod_radii.buffer[i - 1] + 1;
					const int32_t lod_radius_max =
						i == terrain.lod_radii.capacity - 1 ? INT32_MAX : terrain.lod_radii.buffer[i + 1] - 1;

					draw_property_label(lod_radius_label);

					ImGui::PushID(static_cast<int>(i));
					ImGui::DragInt(
						"##LOD radius",
						&terrain.lod_radii.buffer[i],
						0.1f,
						lod_radius_min,
						lod_radius_max,
						"%d",
						ImGuiSliderFlags_AlwaysClamp
					);
					ImGui::PopID();
				}

				draw_property_label("LOD hysteresis");

				ImGui::DragInt(
					"##LOD hysteresis",
					&terrain.lod_hysteresis,
					0.1f,
					0,
					INT32_MAX,
					"%d",
					ImGuiSliderFlags_AlwaysClamp
				);

				ImGui::TreePop();
			}

			// Streamer.

			Stream_Settings& streamer_settings = context.active_world->settings.stream;

			if (ImGui::TreeNodeEx("Stream", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// Each radius is clamped against the other so the unload radius stays greater than the load radius.

				draw_property_label("Load radius");

				ImGui::DragInt(
					"##Load radius",
					&streamer_settings.load_radius,
					0.1f,
					0,
					streamer_settings.unload_radius - 1,
					"%d",
					ImGuiSliderFlags_AlwaysClamp
				);

				draw_property_label("Unload radius");

				ImGui::DragInt(
					"##Unload radius",
					&streamer_settings.unload_radius,
					0.1f,
					streamer_settings.load_radius + 1,
					INT32_MAX,
					"%d",
					ImGuiSliderFlags_AlwaysClamp
				);

				draw_property_label("Rebase distance");

				ImGui::DragInt(
					"##Rebase distance",
					&streamer_settings.rebase_distance,
					0.1f,
					1,
					INT32_MAX,
					"%d",
					ImGuiSliderFlags_AlwaysClamp
				);

				draw_property_label("Max loads per update");

				ImGui::DragInt(
					"##Max loads per update",
					&streamer_settings.max_loads_per_update,
					0.1f,
					1,
					INT32_MAX,
					"%d",
					ImGuiSliderFlags_AlwaysClamp
				);

				ImGui::TreePop();
			}

			ImGui::EndTabItem();
		}

		// Specific settings for material mode.

		if (context.mode == Editor_Mode::MATERIAL)
		{
			// Material settings.

			if (ImGui::BeginTabItem("Material"))
			{
				// Albedo.

				static char albedo_stem[MAX_RESOURCE_STEM_SIZE] = {};
				draw_property_label("Albedo stem");
				ImGui::InputText("##albedo_stem", albedo_stem, MAX_RESOURCE_STEM_SIZE);

				// Normal.

				static char normal_stem[MAX_RESOURCE_STEM_SIZE] = {};
				draw_property_label("Normal stem");
				ImGui::InputText("##normal_stem", normal_stem, MAX_RESOURCE_STEM_SIZE);

				// ORM.

				static char orm_stem[MAX_RESOURCE_STEM_SIZE] = {};
				draw_property_label("ORM stem");
				ImGui::InputText("##orm_stem", orm_stem, MAX_RESOURCE_STEM_SIZE);

				ImGui::Spacing();

				if (ImGui::Button("Preview"))
				{
					if (Node* node = get_node(context.active_world->scene_graph, context.selected_node_handle))
					{
						if (node->type == Node_Type::MESH_REF)
						{
							Material material = {};

							// Load each texture and initialize the material at runtime.

							if (const Pool_Handle<Texture> albedo = load_texture(albedo_stem);
								albedo != POOL_HANDLE_NONE<Texture>)
							{
								material.albedo = albedo;
							}
							else
							{
								BLK_ERROR("Failed to load albedo");
							}

							if (const Pool_Handle<Texture> normal = load_texture(normal_stem);
								normal != POOL_HANDLE_NONE<Texture>)
							{
								material.normal = normal;
							}
							else
							{
								BLK_ERROR("Failed to load normal");
							}

							if (const Pool_Handle<Texture> orm = load_texture(orm_stem);
								orm != POOL_HANDLE_NONE<Texture>)
							{
								material.orm = orm;
							}
							else
							{
								BLK_ERROR("Failed to load ORM");
							}

							// Load runtime material and assign its handle to the node.
							node->mesh_ref.material_handle = load_material(material);
						}
						else
						{
							BLK_ERROR("Node should be a `MESH_REF`");
						}
					}
					else
					{
						BLK_ERROR("Select a node first");
					}
				}

				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

namespace
{
void
draw_property_label(const char* label)
{
	// Separate the label from the widget above it, so it reads as part of the widget below.
	ImGui::Separator();
	ImGui::TextUnformatted(label);
	ImGui::SetNextItemWidth(-FLT_MIN);
}
}  // namespace
