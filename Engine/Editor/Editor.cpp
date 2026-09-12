// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Editor.hpp"

#include "Engine/Editor/Camera.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Menu.hpp"
#include "Engine/Editor/Status_Bar.hpp"
#include "Engine/Editor/World/Stats.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Event.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Platform/Window_Internal.hpp"
#include "Engine/Renderer/Renderer_Internal.hpp"

#include <Windows.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

#include <stdio.h>

blk::Result
blk::create_editor(Editor_Context& context)
{
	// Create ImGui context.

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	// Set ImGui flags.

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	// Load editor fonts.

	// Format the absolute path to the JetBrains Mono fonts.
	char mono_font_path[MAX_PATH_SIZE];

	if (const int written = snprintf(
			mono_font_path,
			MAX_PATH_SIZE,
			"%s/Fonts/JetBrainsMono-2.304/fonts/ttf",
			BLK_EDITOR_RESOURCES_DIRECTORY
		);
		written < 0 || static_cast<size_t>(written) >= MAX_PATH_SIZE)
	{
		BLK_ERROR("Failed to format the absolute path to JetBrains Mono fonts\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Format the absolute path for the JetBrains Mono Medium font.
	char mono_font_medium_path[MAX_PATH_SIZE];

	if (const int written =
			snprintf(mono_font_medium_path, MAX_PATH_SIZE, "%s/JetBrainsMono-Medium.ttf", mono_font_path);
		written < 0 || static_cast<size_t>(written) >= MAX_PATH_SIZE)
	{
		BLK_ERROR("Failed to format the absolute path to JetBrains Mono Medium font\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Format the absolute path to the material design icons fonts.
	char icon_font_path[MAX_PATH_SIZE];

	if (const int written = snprintf(
			icon_font_path,
			MAX_PATH_SIZE,
			"%s/Fonts/material-design-icons/font",
			BLK_EDITOR_RESOURCES_DIRECTORY
		);
		written < 0 || static_cast<size_t>(written) >= MAX_PATH_SIZE)
	{
		BLK_ERROR("Failed to format the absolute path to the material design icons fonts\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Format the absolute path to the material design icons regular font.
	char icon_font_regular_path[MAX_PATH_SIZE];

	if (const int written =
			snprintf(icon_font_regular_path, MAX_PATH_SIZE, "%s/MaterialIcons-Regular.ttf", icon_font_path);
		written < 0 || static_cast<size_t>(written) >= MAX_PATH_SIZE)
	{
		BLK_ERROR("Failed to format the absolute path to the material design icons regular font\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Set the main editor font.
	context.main_font = io.Fonts->AddFontFromFileTTF(mono_font_medium_path);

	if (!context.main_font)
	{
		BLK_ERROR("Failed to load the main editor font from `%s`\n", mono_font_medium_path);

		return Result::OS_ERROR;
	}

	// Set icon config.

	ImFontConfig icons_config = {};
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	icons_config.GlyphMinAdvanceX = 17.0f;
	icons_config.GlyphOffset.y = 6.0f;
	icons_config.SizePixels = 17.0f;

	io.Fonts->AddFontFromFileTTF(icon_font_regular_path, 0.0f, &icons_config);

	// `PushFont` would stay on the font stack forever because there is no frame to pop it in, so we set the default
	// font instead. `ImGui::NewFrame` picks it up every frame.
	io.FontDefault = context.main_font;

	const Window* window = get_window();

	if (!BLK_VERIFY(window))
	{
		BLK_ERROR("Failed to create editor; window is not created properly\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Set DPI config.

	// TODO (Consistency): This is leaking outside of `Platform/`. It is Win32 specific.
	const float dpi_scale = ImGui_ImplWin32_GetDpiScaleForHwnd(window->hwnd);
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(dpi_scale);
	style.FontScaleDpi = dpi_scale;
	style.FontSizeBase = 17.0f;

	if (!ImGui_ImplWin32_Init(window->hwnd))
	{
		BLK_ERROR("Failed to create ImGui Win32 backend\n");

		return Result::DEVICE_ERROR;
	}

	// Set renderer info.

	ImGui_ImplVulkan_InitInfo init_info = get_renderer_imgui_init_info();

	if (!ImGui_ImplVulkan_Init(&init_info))
	{
		BLK_ERROR("Failed to create ImGui Vulkan backend\n");

		return Result::DEVICE_ERROR;
	}

	return Result::SUCCESS;
}

void
blk::update_editor(Editor_Context& context, const double delta_time)
{
	if (!context.input_state)
	{
		BLK_ERROR("Failed to update editor; input state is missing\n");

		return;
	}

	// Holding mouse right button activates free fly mode.
	bool should_enter_free_fly_mode = is_key_held(*context.input_state, Key::MOUSE_RIGHT);
	bool should_exit_free_fly_mode = is_key_release(*context.input_state, Key::MOUSE_RIGHT);

	// We assign these variables later depending on the `Editor_Mode`.
	// We use them to update the editor camera.
	World* world_mode = nullptr;
	Pool_Handle<Camera> editor_camera_handle = {};

	if (context.mode == Editor_Mode::WORLD)
	{
		// We only enter or exit fly mode is the editor camera is active; not gameplay camera.
		should_enter_free_fly_mode = should_enter_free_fly_mode && context.world_context.is_editor_camera_active;
		should_exit_free_fly_mode = should_exit_free_fly_mode && context.world_context.is_editor_camera_active;

		world_mode = context.world_context.game_world;
		editor_camera_handle = context.world_context.editor_camera_handle;

		// Pause or resume the game simulation.
		if (is_key_press(*context.input_state, Key::KEYBOARD_F1))
		{
			context.world_context.is_game_simulation_paused = !context.world_context.is_game_simulation_paused;
		}

		// Switch between the editor and the in-game camera.
		if (is_key_press(*context.input_state, Key::KEYBOARD_F2) && BLK_VERIFY(context.world_context.game_world))
		{
			if (context.world_context.is_editor_camera_active)
			{
				context.world_context.game_world->active_camera_handle = context.world_context.game_camera_handle;
				context.world_context.is_editor_camera_active = false;
			}
			else
			{
				activate_world_mode_editor_camera(context.world_context);
			}
		}
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
		// If we are not in free fly mode already.
		if (!context.viewport_context.is_in_free_fly_mode)
		{
			// We save the cursor position before hide it to restore it when we exit fly mode.
			BLK_IF_NOT_SUCCESS(get_cursor_position(context.viewport_context.cursor_position_before_hidden))
			{
				BLK_ERROR("Failed to get cursor position\n");

				return;
			}

			hide_cursor();
		}

		context.viewport_context.is_in_free_fly_mode = true;
		update_editor_camera(*world_mode, editor_camera_handle, *context.input_state, delta_time);
	}

	// Exit free-fly mode.
	if (should_exit_free_fly_mode)
	{
		// If we are in fly mode.
		if (context.viewport_context.is_in_free_fly_mode)
		{
			// Set the cursor position to where it was before entering fly mode and show the cursor.
			BLK_IF_NOT_SUCCESS(set_cursor_position(context.viewport_context.cursor_position_before_hidden))
			{
				BLK_ERROR("Failed to set cursor position\n");

				return;
			}

			show_cursor();
		}

		context.viewport_context.is_in_free_fly_mode = false;
	}

	// Stuff needed by ImGui using a Vulkan backend.

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// We first compute data that the editor needs.
	compute_stats(context, delta_time);

	// Draw the status bar now because it needs data compute previously.
	draw_status_bar(context);

	// The menu is the source of truth of the editor; it is from where all widgets are drawn, shown, or hidden.
	draw_menu(context);

	// We finally tell ImGui to render everything.
	ImGui::Render();
}

void
blk::destroy_editor()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// We do not destroy `Editor_Context` here because it is not created by `create_editor`. Instead, it is created by
	// the caller.
}
