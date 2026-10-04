// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Editor/Editor.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Core/String.hpp"
#include "Engine/Editor/Camera.hpp"
#include "Engine/Editor/Context.hpp"
#include "Engine/Editor/Menu.hpp"
#include "Engine/Editor/Status_Bar.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Platform/Application.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Event.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Platform/Window_Internal.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Renderer_Internal.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Scene/Node.hpp"
#include "Stats.hpp"

#include <Windows.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

#include <stdio.h>

namespace
{
blk::Editor_Context context = {};
}  // namespace

blk::Result
blk::create_editor(World* game_world, Allocator* allocator)
{
	if (!BLK_VERIFY(game_world) || !BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	// Assign game world.
	context.world_context.world = game_world;

	// Create the editor camera for `Editor_Mode::WORLD` and activate it by default.

	context.world_context.editor_camera_handle = spawn_camera(*game_world, game_world->scene_graph.root);

	if (!BLK_VERIFY(context.world_context.editor_camera_handle != POOL_HANDLE_NONE<Camera>))
	{
		return Result::INVALID_ARGUMENTS;
	}

	context.world_context.world->active_camera_handle = context.world_context.editor_camera_handle;
	Camera* editor_camera = get_camera(*game_world, context.world_context.editor_camera_handle);
	BLK_CHECK(editor_camera);

	editor_camera->near_plane = 0.5f;
	editor_camera->far_plane = 5'000.0f;

	BLK_IF_NOT_SUCCESS(rename_node(game_world->scene_graph, editor_camera->node_handle, "Editor_Camera"))
	{
		BLK_ERROR("Failed to rename editor camera\n");
	}

	// Create the world for `Editor_Mode::MATERIAL`.
	BLK_CHECK(create_world(allocator, {}, context.material_context.world) == Result::SUCCESS);

	// Spawn camera.

	const Pool_Handle<Camera> material_mode_camera_handle = spawn_camera(context.material_context.world, {});
	context.material_context.world.active_camera_handle = material_mode_camera_handle;
	Camera* material_mode_camera = get_camera(context.material_context.world, material_mode_camera_handle);
	BLK_CHECK(material_mode_camera);

	BLK_CHECK(
		rename_node(context.material_context.world.scene_graph, material_mode_camera->node_handle, "Editor_Camera") ==
		Result::SUCCESS
	);

	// Spawn UV sphere.

	const Pool_Handle<Mesh> sphere_mesh_handle = compute_uv_sphere(1.0f, 64, 32);
	const Pool_Handle<Node> sphere_node_handle =
		spawn_node(context.material_context.world.scene_graph, Node_Type::MESH_REF, {});

	BLK_CHECK(rename_node(context.material_context.world.scene_graph, sphere_node_handle, "Sphere") == Result::SUCCESS);

	Node* sphere_node = get_node(context.material_context.world.scene_graph, sphere_node_handle);
	BLK_CHECK(sphere_node);

	sphere_node->transform.position.z = 5.0f;
	sphere_node->mesh_ref.mesh_handle = sphere_mesh_handle;

	// Create the world for `Editor_Mode::PREFAB`.

	BLK_CHECK(create_world(allocator, {}, context.prefab_context.world) == Result::SUCCESS);

	// Spawn camera.
	const Pool_Handle<Camera> prefab_mode_camera_handle = spawn_camera(context.prefab_context.world, {});
	context.prefab_context.world.active_camera_handle = prefab_mode_camera_handle;

	Camera* prefab_mode_camera = get_camera(context.material_context.world, material_mode_camera_handle);
	BLK_CHECK(prefab_mode_camera);

	BLK_CHECK(
		rename_node(context.prefab_context.world.scene_graph, prefab_mode_camera->node_handle, "Editor_Camera") ==
		Result::SUCCESS
	);

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

	BLK_IF_NOT_SNPRINTF(
		mono_font_path,
		MAX_PATH_SIZE,
		"%s/Fonts/JetBrainsMono-2.304/fonts/ttf",
		BLK_EDITOR_RESOURCES_DIRECTORY
	)
	{
		BLK_ERROR("Failed to format the absolute path to JetBrains Mono fonts\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Format the absolute path for the JetBrains Mono Medium font.
	char mono_font_medium_path[MAX_PATH_SIZE];

	BLK_IF_NOT_SNPRINTF(mono_font_medium_path, MAX_PATH_SIZE, "%s/JetBrainsMono-Medium.ttf", mono_font_path)
	{
		BLK_ERROR("Failed to format the absolute path to JetBrains Mono Medium font\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Format the absolute path to the material design icons fonts.
	char icon_font_path[MAX_PATH_SIZE];

	BLK_IF_NOT_SNPRINTF(
		icon_font_path,
		MAX_PATH_SIZE,
		"%s/Fonts/material-design-icons/font",
		BLK_EDITOR_RESOURCES_DIRECTORY
	)
	{
		BLK_ERROR("Failed to format the absolute path to the material design icons fonts\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Format the absolute path to the material design icons regular font.
	char icon_font_regular_path[MAX_PATH_SIZE];

	BLK_IF_NOT_SNPRINTF(icon_font_regular_path, MAX_PATH_SIZE, "%s/MaterialIcons-Regular.ttf", icon_font_path)
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

blk::Editor_Context
blk::update_editor(const Input_State& input_state, const double delta_time)
{
	// We only handle global input here and delegates the rest to its own widgets.

	// Set active world based on mode.

	switch (context.mode)
	{
	case Editor_Mode::WORLD: {
		context.active_world = context.world_context.world;
	}
	break;
	case Editor_Mode::MATERIAL: {
		context.active_world = &context.material_context.world;
	}
	break;
	case Editor_Mode::PREFAB: {
		context.active_world = &context.prefab_context.world;
	}
	break;
	}

	// Holding mouse right button activates free fly mode.
	const bool should_enter_free_fly_mode = is_key_held(input_state, Key::MOUSE_RIGHT);
	const bool should_exit_free_fly_mode = is_key_release(input_state, Key::MOUSE_RIGHT);

	// Enter free-fly mode.
	if (should_enter_free_fly_mode)
	{
		// If we are not in free fly mode already.
		if (!context.is_in_free_fly_mode)
		{
			// We save the cursor position before hide it to restore it when we exit fly mode.
			BLK_IF_NOT_SUCCESS(get_cursor_position(context.cursor_position_x, context.cursor_position_y))
			{
				BLK_ERROR("Failed to get cursor position\n");

				return context;
			}

			hide_cursor();
		}

		context.is_in_free_fly_mode = true;
		update_editor_camera(context, input_state, delta_time);
	}

	// Exit free-fly mode.
	if (should_exit_free_fly_mode)
	{
		// If we are in fly mode.
		if (context.is_in_free_fly_mode)
		{
			// Set the cursor position to where it was before entering fly mode and show the cursor.
			BLK_IF_NOT_SUCCESS(set_cursor_position(context.cursor_position_x, context.cursor_position_y))
			{
				BLK_ERROR("Failed to set cursor position\n");

				return context;
			}

			show_cursor();
		}

		context.is_in_free_fly_mode = false;
	}

	// Stuff needed by ImGui using a Vulkan backend.

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// We first compute data that the editor needs.
	compute_stats(context, delta_time);

	// The menu is the source of truth of the editor; it is from where all widgets are drawn, shown, or hidden.
	draw_menu(context);

	// Draw the status bar now because it needs data compute previously.
	draw_status_bar(context);

	// We finally tell ImGui to render everything.
	ImGui::Render();

	return context;
}

bool
blk::is_game_simulation_paused()
{
	return context.world_context.is_game_simulation_paused;
}

blk::World*
blk::get_active_world()
{
	return context.active_world;
}

void
blk::destroy_editor()
{
	// `ImGui_ImplVulkan_Shutdown` destroys GPU resources, so we wait for the GPU to finish.
	wait_renderer_idle();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
