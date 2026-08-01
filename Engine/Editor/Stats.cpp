// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Stats.hpp"

#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Helpers.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Scene/Mesh_Instance.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/World/World.hpp"

#include <imgui.h>

namespace
{
constexpr int FRAME_SAMPLE_COUNT = 120;

struct Performance_Sample
{
	float fps_count;
	float ms_count;
};

struct World_Sample
{
	uint32_t vertex_count;
	uint32_t index_count;
	uint32_t node_count;
};

Performance_Sample performance_samples[FRAME_SAMPLE_COUNT] = {};
size_t performance_sample_offset = 0;

// Smooth values to improve readability.
constexpr double LAST_REFRESH_THRESHOLD = 0.25;

Performance_Sample sum_since_last_refresh = {};
Performance_Sample displayed_sample = {};
unsigned frame_count_since_last_refresh = 0;
double time_since_last_refresh = 0.0;

World_Sample world_sample = {};
}  // namespace

void
blk::draw_stats(Editor_Context& context)
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	const float available_height = viewport->WorkSize.y - context.status_bar_size.y;

	context.stats_size.x = context.left_column_width;
	context.stats_size.y = available_height - context.scene_graph_size.y;

	context.stats_min_size = Rect{context.scene_graph_min_size.x, context.stats_size.y};
	context.stats_max_size = Rect{context.scene_graph_max_size.x, context.stats_size.y};

	context.stats_position.x = viewport->WorkPos.x;
	context.stats_position.y = viewport->WorkPos.y + context.scene_graph_size.y;

	ImGui::SetNextWindowPos(to_imvec2(context.stats_position), ImGuiCond_Always);
	ImGui::SetNextWindowSize(to_imvec2(context.stats_size), ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(to_imvec2(context.stats_min_size), to_imvec2(context.stats_max_size));

	ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoMove);
	context.left_column_width = ImGui::GetWindowWidth();

	// ============================================================================
	// World.
	// ============================================================================

	ImGui::SeparatorText("World");

	ImGui::Text("Node count: %u", world_sample.node_count);
	ImGui::Text("Vertex count: %i", world_sample.vertex_count);
	ImGui::Text("Index count: %i", world_sample.index_count);

	// ============================================================================
	// Performance.
	// ============================================================================

	ImGui::SeparatorText("Performance");

	draw_fps();

	ImGui::PlotLines(
		"##fps",
		&performance_samples[0].fps_count,
		FRAME_SAMPLE_COUNT,
		static_cast<int>(performance_sample_offset),
		"FPS",
		FLT_MAX,
		FLT_MAX,
		ImVec2(0, 120),
		sizeof(Performance_Sample)
	);

	ImGui::Separator();
	draw_ms();

	ImGui::PlotLines(
		"##ms",
		&performance_samples[0].ms_count,
		FRAME_SAMPLE_COUNT,
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
	// ============================================================================
	// World.
	// ============================================================================

	world_sample = {};

	world_sample.node_count = context.world->scene_graph.nodes.count();

	for (const auto& node : context.world->scene_graph.nodes)
	{
		if (node.mesh_instance)
		{
			const Mesh_Instance mesh_instance = node.mesh_instance.value();
			const Mesh* mesh = get_mesh(mesh_instance.mesh_handle);
			BLK_CHECK(mesh);
			world_sample.vertex_count += mesh->vertices.size();
			world_sample.index_count += mesh->indices.size();
		}
	}

	// ============================================================================
	// Performance.
	// ============================================================================

	Performance_Sample sample = {};
	sample.fps_count = delta_time > 0.0 ? static_cast<float>(1.0 / delta_time) : 0.0f;
	sample.ms_count = static_cast<float>(delta_time) * 1000.0f;

	sum_since_last_refresh.fps_count += sample.fps_count;
	sum_since_last_refresh.ms_count += sample.ms_count;

	frame_count_since_last_refresh += 1;
	time_since_last_refresh += delta_time;

	if (time_since_last_refresh >= LAST_REFRESH_THRESHOLD)
	{
		displayed_sample.fps_count =
			sum_since_last_refresh.fps_count / static_cast<float>(frame_count_since_last_refresh);
		displayed_sample.ms_count =
			sum_since_last_refresh.ms_count / static_cast<float>(frame_count_since_last_refresh);

		performance_samples[performance_sample_offset] = displayed_sample;
		performance_sample_offset = (performance_sample_offset + 1) % FRAME_SAMPLE_COUNT;

		sum_since_last_refresh = {};
		frame_count_since_last_refresh = 0;
		time_since_last_refresh = 0.0;
	}
}
