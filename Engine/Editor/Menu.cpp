// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Menu.hpp"

#include "Engine/Editor/Console.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Material/Settings.hpp"
#include "Engine/Editor/Toolbar.hpp"
#include "Engine/Editor/World/Outliner.hpp"
#include "Engine/Editor/World/Stats.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/World/World.hpp"

#include <imgui.h>

void
blk::draw_menu(Editor_Context& context)
{
	if (!BLK_VERIFY(context.world_context.game_world) || !BLK_VERIFY(context.input_state))
	{
		return;
	}

	// First we draw the options and store the action.

	if (ImGui::BeginMainMenuBar())
	{
		// File category.
		// Anything related to create, or open files that affect the entire engine context should be here.

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

		// Edit category.
		// Anything related to editting the general engine context.

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

		// View category.
		// Anything related to showing, or hiding widgets.

		if (ImGui::BeginMenu("View"))
		{
			const bool show_all = context.world_context.show_outliner && context.world_context.show_stats &&
								  context.world_context.show_console;
			const bool hide_all = !context.world_context.show_outliner && !context.world_context.show_stats &&
								  !context.world_context.show_console;

			if (ImGui::MenuItem("Show all", "Ctrl+Shift+H", show_all))
			{
				context.show_toolbar = true;

				context.world_context.show_console = true;
				context.world_context.show_outliner = true;
				context.world_context.show_stats = true;

				context.material_context.show_material_settings = true;
			}

			if (ImGui::MenuItem("Hide all", "Ctrl+H", hide_all))
			{
				context.show_toolbar = false;

				context.world_context.show_console = false;
				context.world_context.show_outliner = false;
				context.world_context.show_stats = false;

				context.material_context.show_material_settings = false;
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

		// Help category.
		// Anything related to helping the user use the engine.

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About", nullptr))
			{
				BLK_NOT_IMPLEMENTED();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	// Then we draw widgets based on the user's intentions stored previously.

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
