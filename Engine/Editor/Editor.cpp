// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Editor.hpp"

#include "Engine/Editor/Camera.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Menu.hpp"
#include "Engine/Editor/Status_Bar.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Event.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Window_Internal.hpp"
#include "Engine/Renderer/Renderer_Internal.hpp"
#include "World/Stats.hpp"

#include <Windows.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

#include <format>
#include <string>

void
blk::create_editor(Editor_Context& context)
{
	BLK_CHECK(context.world_context._game_world);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	const std::string jetbrain_mono_font_path =
		std::format("{}/Fonts/JetBrainsMono-2.304/fonts/ttf", BLK_EDITOR_RESOURCES_DIRECTORY);
	const std::string material_design_icons_path =
		std::format("{}/Fonts/material-design-icons/font", BLK_EDITOR_RESOURCES_DIRECTORY);

	const std::string jetbrain_mono_medium_path =
		std::format("{}/JetBrainsMono-Medium.ttf", jetbrain_mono_font_path.c_str());
	const std::string material_design_icons_regular_path =
		std::format("{}/MaterialIcons-Regular.ttf", material_design_icons_path.c_str());

	context.main_font = io.Fonts->AddFontFromFileTTF(jetbrain_mono_medium_path.c_str());

	ImFontConfig icons_config = {};
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	icons_config.GlyphMinAdvanceX = 17.0f;
	icons_config.GlyphOffset.y = 6.0f;
	icons_config.SizePixels = 17.0f;

	io.Fonts->AddFontFromFileTTF(material_design_icons_regular_path.c_str(), 0.0f, &icons_config);
	ImGui::PushFont(context.main_font, 17.0f);

	const Window* window = get_window();
	BLK_CHECK(window);

	// FIXME: This is leaking outside of Platform/
	const float dpi_scale = ImGui_ImplWin32_GetDpiScaleForHwnd(window->hwnd);
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(dpi_scale);
	style.FontScaleDpi = dpi_scale;

	// FIXME: This is leaking outside of Platform/
	if (!ImGui_ImplWin32_Init(window->hwnd))
	{
		BLK_FATAL("Failed to create ImGui Win32 backend\n");
	}

	ImGui_ImplVulkan_InitInfo init_info = get_renderer_imgui_init_info();

	if (!ImGui_ImplVulkan_Init(&init_info))
	{
		BLK_FATAL("Failed to create ImGui Vulkan backend\n");
	}
}

void
blk::update_editor(Editor_Context& context, const double delta_time)
{
	BLK_CHECK(context._input_state);

	bool should_enter_free_fly_mode = is_key_held(*context._input_state, Key::MOUSE_RIGHT);
	bool should_exit_free_fly_mode = is_key_release(*context._input_state, Key::MOUSE_RIGHT);
	World* world_mode = nullptr;
	Pool_Handle<Camera> editor_camera_handle = {};

	if (context.mode == Editor_Mode::WORLD)
	{
		should_enter_free_fly_mode = should_enter_free_fly_mode && context.world_context.is_editor_camera_active;
		should_exit_free_fly_mode = should_exit_free_fly_mode && context.world_context.is_editor_camera_active;
		world_mode = context.world_context._game_world;
		editor_camera_handle = context.world_context.editor_camera_handle;
	}

	if (context.mode == Editor_Mode::MATERIAL)
	{
		world_mode = &context.material_context.world;
		editor_camera_handle = context.material_context.camera_handle;
	}

	BLK_CHECK(world_mode);

	// Enter free-fly mode.
	if (should_enter_free_fly_mode)
	{
		if (!context.viewport_context.is_in_free_fly_mode)
		{
			context.viewport_context.cursor_position_before_hidden = get_cursor_position();
			hide_cursor();
		}

		context.viewport_context.is_in_free_fly_mode = true;
		update_editor_camera(*world_mode, editor_camera_handle, *context._input_state, delta_time);
	}

	// Exit free-fly mode.
	if (should_exit_free_fly_mode)
	{
		if (context.viewport_context.is_in_free_fly_mode)
		{
			set_cursor_position(context.viewport_context.cursor_position_before_hidden);
			show_cursor();
		}

		context.viewport_context.is_in_free_fly_mode = false;
	}

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	compute_stats(context, delta_time);

	draw_status_bar(context);
	draw_menu(context);

	ImGui::Render();
}

void
blk::destroy_editor()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
