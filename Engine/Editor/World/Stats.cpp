// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/World/Stats.hpp"

#include "Engine/Editor/Context.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Scene/Mesh_Instance.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/World.hpp"

#include <imgui.h>

#include <float.h>

namespace
{
/// Number of samples stored.
constexpr int SAMPLE_COUNT = 120;

/// A performance sample.
struct Performance_Sample
{
	/// Frames per second.
	float fps_count;
	/// Miliseconds per frame.
	float ms_count;
};

/// Information about the world.
struct World_Sample
{
	/// Number of vertices in the world.
	uint32_t vertex_count;
	/// Number of indices in the world.
	uint32_t index_count;
	/// Number of nodes in the world.
	uint32_t node_count;
};

/// Array of performance samples.
Performance_Sample performance_samples[SAMPLE_COUNT] = {};
/// Next position in `performance_samples` to write to.
size_t performance_sample_offset = 0;

/// Current world sample.
World_Sample world_sample = {};

/// Threshold in seconds to wait until a new sample is displayed.
/// Data like miliseconds per frame and FPS could vary heavily per frame when we do not have the FPS capped, making this
/// value unreadable if we display it every frame. By waiting some time, we smooth it and improve readability.
constexpr double LAST_REFRESH_THRESHOLD = 0.25;

/// Sum of samples since the last refresh.
Performance_Sample sum_since_last_refresh = {};
/// Currently displayed values.
Performance_Sample displayed_sample = {};
/// How many frames has passed since the last refresh.
unsigned frame_count_since_last_refresh = 0;
/// How much time has passed since the last refresh.
double time_since_last_refresh = 0.0;
}  // namespace

void
blk::draw_stats(Editor_Context& context)
{
	// Get ImGui viewport.

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// The stats widget is at the bottom left of the screen, below the outliner and above the status bar.
	// We compute its size and position.

	const float available_height =
		viewport->WorkSize.y - context.viewport_context.status_bar_size.y - context.toolbar_size.y;

	context.world_context.stats_size.x = context.left_column_width;
	context.world_context.stats_size.y = available_height - context.world_context.scene_graph_size.y;

	context.world_context.stats_min_size =
		ImVec2(context.world_context.scene_graph_min_size.x, context.world_context.stats_size.y);
	context.world_context.stats_max_size =
		ImVec2(context.world_context.scene_graph_max_size.x, context.world_context.stats_size.y);

	context.world_context.stats_position.x = viewport->WorkPos.x;
	context.world_context.stats_position.y =
		viewport->WorkPos.y + context.world_context.scene_graph_size.y + context.toolbar_size.y;

	// Pass size, position, and contraint data to ImGui.

	ImGui::SetNextWindowPos(context.world_context.stats_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.world_context.stats_size, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(context.world_context.stats_min_size, context.world_context.stats_max_size);

	ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoMove);
	context.left_column_width = ImGui::GetWindowWidth();

	// Display world data.

	ImGui::SeparatorText("World");

	ImGui::Text("Node count: %u", world_sample.node_count);
	ImGui::Text("Vertex count: %u", world_sample.vertex_count);
	ImGui::Text("Index count: %u", world_sample.index_count);

	// Display performance data.

	ImGui::SeparatorText("Performance");

	draw_fps();

	// Plot FPS.
	ImGui::PlotLines(
		"##fps",
		&performance_samples[0].fps_count,
		SAMPLE_COUNT,
		static_cast<int>(performance_sample_offset),
		"FPS",
		FLT_MAX,
		FLT_MAX,
		ImVec2(0, 120),
		sizeof(Performance_Sample)
	);

	ImGui::Separator();
	draw_ms();

	// Plot miliseconds per frame.
	ImGui::PlotLines(
		"##ms",
		&performance_samples[0].ms_count,
		SAMPLE_COUNT,
		static_cast<int>(performance_sample_offset),
		"ms",
		FLT_MAX,
		FLT_MAX,
		ImVec2(0, 120),
		sizeof(Performance_Sample)
	);

	ImGui::End();
}

void
blk::draw_fps()
{
	ImGui::Text("FPS: %.1f", displayed_sample.fps_count);
}

void
blk::draw_ms()
{
	ImGui::Text("Frame: %.2f ms", displayed_sample.ms_count);
}

void
blk::compute_stats(const Editor_Context& context, const double delta_time)
{
	if (!BLK_VERIFY(context.world_context.game_world))
	{
		return;
	}

	// Compute world sample first.

	world_sample = {};

	world_sample.node_count = static_cast<uint32_t>(context.world_context.game_world->scene_graph.nodes.count);

	// TODO (Performance): Whe are iterating over the whole capacity of the pool.
	// This is solvable by keeping a packed array inside a `Pool` to iterate over.

	// Iterate over each node to compute total vertices and indices in the scene.

	for (size_t slot_index = 0; slot_index < context.world_context.game_world->scene_graph.nodes.capacity; ++slot_index)
	{
		const Pool_Slot<Node> slot = context.world_context.game_world->scene_graph.nodes.slots[slot_index];

		if (!slot.is_used)
		{
			continue;
		}

		const Node& node = slot.element;

		// We only count vertex and index data in mesh instance nodes.

		if (node.type != Node_Type::MESH_INSTANCE)
		{
			continue;
		}

		for (size_t mesh_index = 0; mesh_index < node.mesh_instance.mesh_handles.count; ++mesh_index)
		{
			const Pool_Handle<Mesh> mesh_handle = node.mesh_instance.mesh_handles.buffer[mesh_index];

			if (mesh_handle == POOL_HANDLE_NONE<Mesh>)
			{
				continue;
			}

			const Mesh* mesh = get_mesh(mesh_handle);

			if (!mesh)
			{
				BLK_ERROR("Failed to get node mesh\n");

				continue;
			}

			// Update the world sample.

			world_sample.vertex_count += static_cast<uint32_t>(mesh->vertices.count);
			world_sample.index_count += static_cast<uint32_t>(mesh->indices.count);
		}
	}

	// Compute performance sample.

	Performance_Sample sample = {};
	sample.fps_count = delta_time > 0.0 ? static_cast<float>(1.0 / delta_time) : 0.0f;
	sample.ms_count = static_cast<float>(delta_time) * 1000.0f;

	// Sum current sample to the total.

	sum_since_last_refresh.fps_count += sample.fps_count;
	sum_since_last_refresh.ms_count += sample.ms_count;

	// Increase frame count and time since last refresh.

	frame_count_since_last_refresh += 1;
	time_since_last_refresh += delta_time;

	// If we surpassed the last refresh threshold, we update our display sample to we drawn in the editor.

	if (time_since_last_refresh >= LAST_REFRESH_THRESHOLD)
	{
		// Compute average per frame of past samples.

		displayed_sample.fps_count =
			sum_since_last_refresh.fps_count / static_cast<float>(frame_count_since_last_refresh);
		displayed_sample.ms_count =
			sum_since_last_refresh.ms_count / static_cast<float>(frame_count_since_last_refresh);

		// Add the display sample to our array.

		performance_samples[performance_sample_offset] = displayed_sample;
		performance_sample_offset = (performance_sample_offset + 1) % SAMPLE_COUNT;

		// Clean up.

		sum_since_last_refresh = {};
		frame_count_since_last_refresh = 0;
		time_since_last_refresh = 0.0;
	}
}
