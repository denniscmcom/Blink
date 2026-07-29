#include "Editor/Editor.hpp"

#include "Context.hpp"
#include "Editor/Camera.hpp"
#include "Editor/Menu.hpp"
#include "Editor/Stats.hpp"
#include "Editor/Status_Bar.hpp"
#include "Input/Input.hpp"
#include "Platform/Application.hpp"
#include "Platform/Log.hpp"
#include "Platform/Window_Internal.hpp"
#include "Renderer/Renderer_Internal.hpp"
#include "World/World.hpp"

#include <Windows.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

void
blk::create_editor(Editor_Context& context)
{
	create_editor_camera(context);
	activate_editor_camera(context);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	Window* window = get_window();
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
blk::update_editor(Editor_Context& context, const double delta_time, const Input_State& input_state)
{
	// Pause/resume game simulation.
	if (is_key_press(input_state, Key::KEYBOARD_F1))
	{
		context.is_game_simulation_paused = !context.is_game_simulation_paused;
	}

	// Editor/game camera.
	if (is_key_press(input_state, Key::KEYBOARD_F2))
	{
		if (context.is_editor_camera_active)
		{
			context.world->active_camera_handle = context.game_camera_handle;
			context.is_editor_camera_active = false;
		}
		else
		{
			activate_editor_camera(context);
		}
	}

	// Enter free-fly mode.
	if (context.is_editor_camera_active && is_key_held(input_state, Key::MOUSE_RIGHT))
	{
		if (!context.is_in_free_fly_mode)
		{
			context.cursor_position_before_hidden = get_cursor_position();
			hide_cursor();
		}

		context.is_in_free_fly_mode = true;
		update_editor_camera(context, delta_time, input_state);
	}

	// Exit free-fly mode.
	if (context.is_editor_camera_active && is_key_release(input_state, Key::MOUSE_RIGHT))
	{
		if (context.is_in_free_fly_mode)
		{
			set_cursor_position(context.cursor_position_before_hidden);
			show_cursor();
		}

		context.is_in_free_fly_mode = false;
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
