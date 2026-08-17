// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/World/Console.hpp"

#include "Engine/Editor/Context.hpp"

#include <imgui.h>

#include <stdio.h>
#include <string.h>

namespace
{
constexpr int MAX_LINE_LENGTH = 1024;
constexpr int MAX_LINE_COUNT = 1024;

struct Console_Log
{
	char lines[MAX_LINE_COUNT][MAX_LINE_LENGTH] = {};
	int line_count = 0;
	int next_line = 0;
	bool auto_scroll = true;
};

Console_Log console_log = {};

ImVec4 get_line_color(const char* line);
}  // namespace

void
blk::log_editor(const char* msg)
{
	char* dst = console_log.lines[console_log.next_line];

	int written = snprintf(dst, MAX_LINE_LENGTH, "%s", msg);

	if (written < 0)
	{
		return;
	}

	if (written >= MAX_LINE_LENGTH)
	{
		written = MAX_LINE_LENGTH - 1;
	}

	// Strip a single trailing newline so each entry renders as one row.
	if (written > 0 && dst[written - 1] == '\n')
	{
		dst[written - 1] = '\0';
	}

	console_log.next_line = (console_log.next_line + 1) % MAX_LINE_COUNT;

	if (console_log.line_count < MAX_LINE_COUNT)
	{
		console_log.line_count += 1;
	}
}

void
blk::draw_console(Editor_Context& context)
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	context.world_context.console_size.x =
		viewport->WorkSize.x - context.world_context.scene_graph_size.x - context.world_context.settings_size.x;
	context.world_context.console_size.y = 650.0f;

	context.world_context.console_position.x = viewport->WorkPos.x + context.world_context.stats_size.x;
	context.world_context.console_position.y = viewport->WorkSize.y - context.world_context.console_size.y;

	ImGui::SetNextWindowPos(context.world_context.console_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.world_context.console_size, ImGuiCond_Always);

	ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	if (ImGui::Button("Clear"))
	{
		console_log.line_count = 0;
		console_log.next_line = 0;
	}

	ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &console_log.auto_scroll);

	ImGui::Separator();

	ImGui::BeginChild("Scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

	// Oldest line index: 0 until the ring wraps, then the current write index.
	const int oldest = console_log.line_count < MAX_LINE_COUNT ? 0 : console_log.next_line;

	ImGuiListClipper clipper;
	clipper.Begin(console_log.line_count);

	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
		{
			const int index = (oldest + row) % MAX_LINE_COUNT;
			const char* line = console_log.lines[index];
			const ImVec4 line_color = get_line_color(line);

			ImGui::PushStyleColor(ImGuiCol_Text, line_color);
			ImGui::TextUnformatted(line);
			ImGui::PopStyleColor();
		}
	}

	clipper.End();

	if (console_log.auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();

	ImGui::End();
}

namespace
{
ImVec4
get_line_color(const char* line)
{
	const char* tag = line + 2;

	if (strncmp(tag, "FATAL", 5) == 0)
	{
		return ImVec4(0.95f, 0.55f, 0.78f, 1.0f);
	}

	if (strncmp(tag, "ERROR", 5) == 0)
	{
		return ImVec4(0.90f, 0.45f, 0.45f, 1.0f);
	}

	if (strncmp(tag, "WARNING", 7) == 0)
	{
		return ImVec4(0.90f, 0.73f, 0.36f, 1.0f);
	}

	if (strncmp(tag, "INFO", 4) == 0)
	{
		return ImVec4(0.80f, 0.83f, 0.88f, 1.0f);
	}

	if (strncmp(tag, "DEBUG", 5) == 0)
	{
		return ImVec4(0.60f, 0.64f, 0.70f, 1.0f);
	}

	if (strncmp(tag, "TRACE", 5) == 0)
	{
		return ImVec4(0.45f, 0.47f, 0.52f, 1.0f);
	}

	return ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
}
}  // namespace
