// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Menu.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Editor/Console.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Scene_Graph.hpp"
#include "Engine/Editor/Settings.hpp"
#include "Engine/Editor/Stats.hpp"
#include "Engine/Editor/Toolbar.hpp"
#include "Engine/Platform/Assert.hpp"

#include <imgui.h>

namespace
{
/// Menu entry that shows or hides a widget.
struct Widget_Toggle
{
	/// Menu item label.
	const char* label;
	/// Visibility flag of the widget.
	bool* is_shown;
	/// Pointer to a function to draw the widget.
	void (*draw)(blk::Editor_Context& context);
	/// Pointer to widget size. Reset when hidden so it does not take up space.
	ImVec2* size;
};
}  // namespace

void
blk::draw_menu(Editor_Context& context)
{
	const Array widget_toggles = {{
		Widget_Toggle{
			.label = "Show toolbar",
			.is_shown = &context.is_showing_toolbar,
			.draw = draw_toolbar,
			.size = &context.toolbar_size,
		},
		Widget_Toggle{
			.label = "Show scene graph",
			.is_shown = &context.is_showing_scene_graph,
			.draw = draw_scene_graph,
			.size = &context.scene_graph_size,
		},
		Widget_Toggle{
			.label = "Show settings",
			.is_shown = &context.is_showing_settings,
			.draw = draw_settings,
			.size = &context.settings_size,
		},
		Widget_Toggle{
			.label = "Show console",
			.is_shown = &context.is_showing_console,
			.draw = draw_console,
			.size = &context.console_size,
		},
		Widget_Toggle{
			.label = "Show stats",
			.is_shown = &context.is_showing_stats,
			.draw = draw_stats,
			.size = &context.stats_size,
		},
	}};

	if (ImGui::BeginMainMenuBar())
	{
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

			bool is_all_shown = true;
			bool is_all_hidden = true;

			for (size_t i = 0; i < widget_toggles.capacity; ++i)
			{
				is_all_shown &= *widget_toggles.buffer[i].is_shown;
				is_all_hidden &= !*widget_toggles.buffer[i].is_shown;
			}

			if (ImGui::MenuItem("Show all", "Ctrl+Shift+H", is_all_shown))
			{
				for (size_t i = 0; i < widget_toggles.capacity; ++i)
				{
					*widget_toggles.buffer[i].is_shown = true;
				}
			}

			if (ImGui::MenuItem("Hide all", "Ctrl+H", is_all_hidden))
			{
				for (size_t i = 0; i < widget_toggles.capacity; ++i)
				{
					*widget_toggles.buffer[i].is_shown = false;
				}
			}

			ImGui::Separator();

			for (size_t i = 0; i < widget_toggles.capacity; ++i)
			{
				ImGui::MenuItem(widget_toggles.buffer[i].label, nullptr, widget_toggles.buffer[i].is_shown);
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	// Widgets are laid out around each other's sizes, so a hidden widget resets its size to not take up space.

	for (size_t i = 0; i < widget_toggles.capacity; ++i)
	{
		if (*widget_toggles.buffer[i].is_shown)
		{
			BLK_CHECK(widget_toggles.buffer[i].draw);
			widget_toggles.buffer[i].draw(context);
		}
		else
		{
			BLK_CHECK(widget_toggles.buffer[i].size);
			*widget_toggles.buffer[i].size = {};
		}
	}
}
