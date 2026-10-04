// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Scene_Graph.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Scene/Graph.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/World.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace
{
/// Helper function to draw the node tree starting at `handle`.
void draw_node(blk::Editor_Context& context, blk::Pool_Handle<blk::Node> handle);
}  // namespace

void
blk::draw_scene_graph(Editor_Context& context)
{
	// Get ImGui viewport.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Compute size and position for the outliner.
	// The outliner is located at the left side of the screen, below the menu bar and above the status bar.

	const float available_height = viewport->WorkSize.y - context.status_bar_size.y - context.toolbar_size.y;

	context.scene_graph_position.x = viewport->WorkPos.x;
	context.scene_graph_position.y = viewport->WorkPos.y + context.toolbar_size.y;
	context.scene_graph_size = ImVec2(context.left_column_width, available_height * 0.6f);
	context.scene_graph_min_size = ImVec2(LEFT_COLUMN_MIN_WIDTH, context.scene_graph_size.y);
	context.scene_graph_max_size = ImVec2(LEFT_COLUMN_MAX_WIDTH, context.scene_graph_size.y);

	// Pass size, position, and constraints to ImGui.

	ImGui::SetNextWindowPos(context.scene_graph_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.scene_graph_size, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(context.scene_graph_min_size, context.scene_graph_max_size);

	// We draw first the scene graph. The scene graph is resizable horizontally by the user within limits.

	ImGui::Begin("Scene graph", nullptr, ImGuiWindowFlags_NoMove);

	// Ask ImGui for the width of the scene graph widget.
	context.left_column_width = ImGui::GetWindowWidth();

	// We draw nodes recursively, starting at the root.
	draw_node(context, context.active_world->scene_graph.root);

	ImGui::End();

	// Compute size and position of the node settings widget.
	// This widget is located at the right of the screen, below the menu and above the status bar.

	context.settings_size = ImVec2(context.right_column_width, available_height);
	context.settings_min_size = ImVec2(RIGHT_COLUMN_MIN_WIDTH, context.settings_size.y);
	context.settings_max_size = ImVec2(RIGHT_COLUMN_MAX_WIDTH, context.settings_size.y);

	context.settings_position.x = viewport->WorkPos.x + viewport->WorkSize.x - context.settings_size.x;
	context.settings_position.y = viewport->WorkPos.y + context.toolbar_size.y;

	// Pass size and position to ImGui.

	ImGui::SetNextWindowPos(context.settings_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.settings_size, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(context.settings_min_size, context.settings_max_size);
}

namespace
{
void
draw_node(blk::Editor_Context& context, const blk::Pool_Handle<blk::Node> handle)
{
	if (!BLK_VERIFY(context.active_world))
	{
		return;
	}

	// Get the node to draw.

	const blk::Node* node = get_node(context.active_world->scene_graph, handle);

	if (!node)
	{
		BLK_ERROR("Failed to find node to draw\n");

		return;
	}

	// Tree node flags.

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

	// If the node to draw is the one currently selected in the outliner, we add the selected flag.

	if (handle == context.selected_node_handle)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	// If the node to draw does not have children, this node is a leaf.

	if (node->first_child_handle == blk::POOL_HANDLE_NONE<blk::Node>)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	// Node names are optional and not unique, so we use the node handle as ImGui ID and the name only as label.
	ImGui::PushID(reinterpret_cast<const void*>(handle.id));

	const char* label = node->name[0] != '\0' ? node->name : "(Unnamed)";
	const bool is_open = ImGui::TreeNodeEx("##Node", flags, "%s", label);

	// Handle node selection.

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		context.selected_node_handle = handle;
	}

	// If the node tree is open, we draw its subtree.

	if (is_open)
	{
		blk::Pool_Handle<blk::Node> child_handle = node->first_child_handle;

		while (const blk::Node* child = get_node(context.active_world->scene_graph, child_handle))
		{
			draw_node(context, child_handle);
			child_handle = child->next_sibling_handle;
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}
}  // namespace
