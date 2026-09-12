// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Console.hpp"

#include "Engine/Editor/Context.hpp"

#include <imgui.h>

#include <stdio.h>
#include <string.h>

namespace
{
/// Maximum characters in a line.
constexpr size_t MAX_LINE_LENGTH = 1024;
/// Maximum lines stored.
/// When this limit is reached, we start overriding the old messages.
constexpr size_t MAX_LINE_COUNT = 1024;

/// Console log context.
struct Console_Log
{
	/// Lines buffer.
	char lines[MAX_LINE_COUNT][MAX_LINE_LENGTH];
	/// Total lines in `lines`.
	size_t line_count;
	/// Next line in `lines` to write to.
	size_t next_line;
	/// When auto scroll is activated new lines will always show automatically.
	bool auto_scroll = true;
};

Console_Log console_log = {};

/// Helper function to color a line based on the log tag (ERROR, WARNING, INFO, etc.).
ImVec4 get_line_color(const char* line);
}  // namespace

void
blk::log_editor(const char* msg)
{
	// Get pointer to the next line to write to.
	char* dst = console_log.lines[console_log.next_line];

	// Write `msg` to `dst`.
	const int written = snprintf(dst, MAX_LINE_LENGTH, "%s", msg);

	if (written < 0 || static_cast<size_t>(written) >= MAX_LINE_LENGTH)
	{
		// An error occurred writing `msg` to `dst`. We cannot use `BLK_VERIFY` here: this function is a log sink, so
		// logging the failure would call back into it and recurse until the stack overflows.
		return;
	}

	// Strip trailing newline so each entry renders as one row.
	if (written > 0 && dst[written - 1] == '\n')
	{
		dst[written - 1] = '\0';
	}

	// Compute next line index. We wrap around if we reach `MAX_LINE_COUNT`.
	console_log.next_line = (console_log.next_line + 1) % MAX_LINE_COUNT;

	if (console_log.line_count < MAX_LINE_COUNT)
	{
		// We sum one to the line count only if the buffer is not already full.
		// Once it is full we just override old lines.
		console_log.line_count += 1;
	}
}

void
blk::draw_console(Editor_Context& context)
{
	// Get ImGui viewport.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Compute console size and position. Console is located at the bottom of the screen above the status bar, and its
	// width is derived by the side widgets.

	context.world_context.console_size.x =
		viewport->WorkSize.x - context.world_context.scene_graph_size.x - context.world_context.settings_size.x;
	context.world_context.console_size.y = 650.0f;

	context.world_context.console_position.x = viewport->WorkPos.x + context.world_context.stats_size.x;
	context.world_context.console_position.y =
		viewport->WorkPos.y + viewport->WorkSize.y - context.world_context.console_size.y;

	// Pass size and position to ImGui.

	ImGui::SetNextWindowPos(context.world_context.console_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(context.world_context.console_size, ImGuiCond_Always);

	// Console is non-movable nor resizable by the user.

	ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	// Clears all the messages in the buffer.
	if (ImGui::Button("Clear"))
	{
		console_log.line_count = 0;
		console_log.next_line = 0;
	}

	ImGui::SameLine();

	// Activate or deactivate auto scroll.
	ImGui::Checkbox("Auto-scroll", &console_log.auto_scroll);

	ImGui::Separator();

	// "Scrolling" is just this child window's ID string, it carries no meaning to ImGui.
	ImGui::BeginChild("Scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

	// 0 until the ring wraps, then the current write index.
	const size_t oldest_line_index = console_log.line_count < MAX_LINE_COUNT ? 0 : console_log.next_line;

	// A clipper skips the rows that fall outside the visible area. It needs the total line count to work out which
	// range that is, so we only pay for the lines actually on screen instead of all `MAX_LINE_COUNT` of them.
	ImGuiListClipper clipper = {};
	clipper.Begin(static_cast<int>(console_log.line_count));

	while (clipper.Step())
	{
		for (int curent_line_index = clipper.DisplayStart; curent_line_index < clipper.DisplayEnd; ++curent_line_index)
		{
			// Compute index in buffer to the current line.
			const size_t index = (oldest_line_index + static_cast<size_t>(curent_line_index)) % MAX_LINE_COUNT;

			// Get a pointer to the current line.
			const char* line = console_log.lines[index];

			// We color the line based on the log tag (ERROR, WARNING, etc.).
			const ImVec4 line_color = get_line_color(line);

			// Pass line style to ImGui.
			ImGui::PushStyleColor(ImGuiCol_Text, line_color);
			ImGui::TextUnformatted(line);
			ImGui::PopStyleColor();
		}
	}

	clipper.End();

	// If auto scroll is activated and we are not at the bottom of the scrolling console.
	if (console_log.auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		// ImGui scrolling goes from 0.0 to 1.0, being 0.0 at the top, and 1.0 at the bottom. So here, we scroll at the
		// bottom so we see new messages in the console.
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
	// We want to get just the log tag to compare it and color the line correctly.
	// Log messages start like this: "[ INFO ] Hello", so to get just "INFO", we position our pointer at index 2 –
	// skipping '[' and ' '. A line shorter than that has no tag, so we use the default color.
	if (strnlen(line, 2) < 2)
	{
		return ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
	}

	const char* tag = line + 2;

	// Now we compare the tag and return the correspondent color.
	// Notice that if we add more log tags in the future we have to update this implementation too.

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

	// This is just a default color for simple messages that does not use a log tag.
	return ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
}
}  // namespace
