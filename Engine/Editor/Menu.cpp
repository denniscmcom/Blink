// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Menu.hpp"

#include "Engine/Editor/Console.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Material_Creator.hpp"
#include "Engine/Editor/Outliner.hpp"
#include "Engine/Editor/Stats.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Scene/Scene_Graph.hpp"
#include "Engine/World/World.hpp"

#include <imgui.h>

void
blk::draw_menu(Editor_Context& context)
{
	BLK_CHECK(context.world);

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Create project"))
			{
			}

			if (ImGui::MenuItem("Load project"))
			{
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Create world"))
			{
			}

			if (ImGui::MenuItem("Save world"))
			{
			}

			if (ImGui::MenuItem("Load world"))
			{
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Create material"))
			{
				context.show_material_creator = true;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("World"))
		{
			if (ImGui::MenuItem("Spawn actor"))
			{
				std::string actor_name = make_unique_node_name(context.world->scene_graph, "Actor");
				spawn_actor(*context.world, actor_name.c_str(), context.selected_node_handle);
			}

			if (ImGui::MenuItem("Spawn camera"))
			{
				std::string camera_name = make_unique_node_name(context.world->scene_graph, "Camera");
				spawn_actor(*context.world, camera_name.c_str(), context.selected_node_handle);
			}

			if (ImGui::MenuItem("Spawn prop"))
			{
				std::string prop_name = make_unique_node_name(context.world->scene_graph, "Prop");
				spawn_actor(*context.world, prop_name.c_str(), context.selected_node_handle);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Despawn selected node"))
			{
				despawn_node(*context.world, context.selected_node_handle);
				context.selected_node_handle = {};
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
		{
			const bool show_all = context.show_outliner && context.show_stats;
			const bool hide_all = !context.show_outliner && !context.show_stats;

			if (ImGui::MenuItem("Show all", nullptr, show_all))
			{
				context.show_outliner = true;
				context.show_stats = true;
				context.show_console = true;
			}

			if (ImGui::MenuItem("Hide all", nullptr, hide_all))
			{
				context.show_outliner = false;
				context.show_stats = false;
				context.show_console = false;
			}

			ImGui::Separator();

			ImGui::MenuItem("Show outliner", nullptr, &context.show_outliner);
			ImGui::MenuItem("Show stats", nullptr, &context.show_stats);
			ImGui::MenuItem("Show console", nullptr, &context.show_console);

			ImGui::EndMenu();
		}
	}

	ImGui::EndMainMenuBar();

	if (context.show_outliner)
	{
		draw_outliner(context);
	}

	if (context.show_stats)
	{
		draw_stats(context);
	}

	if (context.show_console)
	{
		draw_console(context);
	}

	if (context.show_material_creator)
	{
		draw_material_creator(context);
	}
}
