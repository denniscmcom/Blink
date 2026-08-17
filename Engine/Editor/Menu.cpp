// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Menu.hpp"

#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Material/Settings.hpp"
#include "Engine/Editor/Toolbar.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Scene/Scene_Graph.hpp"
#include "Engine/World/World.hpp"
#include "World/Console.hpp"
#include "World/Outliner.hpp"
#include "World/Stats.hpp"
#include "imgui_internal.h"

#include <imgui.h>

void
blk::draw_menu(Editor_Context& context)
{
	BLK_CHECK(context.world_context._game_world);
	BLK_CHECK(context._input_state);

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Create project", "Ctrl+N"))
			{
				BLK_NOT_IMPLEMENTED();
			}

			if (ImGui::MenuItem("Load project", "Ctrl+O"))
			{
				BLK_NOT_IMPLEMENTED();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z"))
			{
				BLK_NOT_IMPLEMENTED();
			}

			if (ImGui::MenuItem("Redo", "Ctrl+R"))
			{
				BLK_NOT_IMPLEMENTED();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			const bool show_all = context.world_context.show_outliner && context.world_context.show_stats;
			const bool hide_all = !context.world_context.show_outliner && !context.world_context.show_stats;

			if (ImGui::MenuItem("Show all", "Ctrl+Shift+H", show_all))
			{
				// TODO (WIP).
				BLK_NOT_IMPLEMENTED();
			}

			if (ImGui::MenuItem("Hide all", "Ctrl+H", hide_all))
			{
				// TODO (WIP).
				BLK_NOT_IMPLEMENTED();
			}

			ImGui::Separator();

			switch (context.mode)
			{
			case Editor_Mode::WORLD:
				ImGui::MenuItem("Show outliner", nullptr, &context.world_context.show_outliner);
				ImGui::MenuItem("Show stats", nullptr, &context.world_context.show_stats);
				ImGui::MenuItem("Show console", nullptr, &context.world_context.show_console);
				break;
			case Editor_Mode::MATERIAL:
				ImGui::MenuItem("Show material settings", nullptr, &context.material_context.show_material_settings);
				break;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About", nullptr))
			{
				BLK_NOT_IMPLEMENTED();
			}

			ImGui::EndMenu();
		}
	}

	ImGui::EndMainMenuBar();

	if (context.show_toolbar)
	{
		draw_toolbar(context);
	}

	switch (context.mode)
	{
	case Editor_Mode::WORLD: {
		if (context.world_context.show_outliner)
		{
			draw_outliner(context);
		}

		if (context.world_context.show_stats)
		{
			draw_stats(context);
		}

		if (context.world_context.show_console)
		{
			draw_console(context);
		}
	}
	break;
	case Editor_Mode::MATERIAL: {
		if (context.material_context.show_material_settings)
		{
			draw_material_settings(context);
		}
	}
	break;
	}
}
