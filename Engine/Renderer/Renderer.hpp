// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Platform/Application.hpp"

namespace blk
{
struct World_Settings;
struct Scene_Graph;
enum class Result;

/// View data to update the frame to render.
struct Renderer_Settings
{
	/// Camera view point.
	Vector3 view_position;
	/// Camera view matrix.
	Matrix4 view;
	/// Camera projection matrix.
	Matrix4 projection;

	/// Render mesh triangles.
	bool is_debug_triangle_enabled;
};

/// Creates the renderer.
Result create_renderer(size_t width, size_t height);
/// Precomputes the renderer's static lookup tables, and binds the resources `settings` shares with every frame.
/// @warning Call it before the first `render_frame`, and again whenever the terrain heightmap in `settings` changes.
/// It waits for the device to be idle.
Result bake_renderer(const World_Settings& settings);
/// Waits until the device has finished every submitted command.
///
/// `render_frame` returns as soon as the frame is submitted, so the GPU is still reading the resources of the last
/// `MAX_FRAMES_IN_FLIGHT` frames after the main loop exits. Anything that destroys a resource those frames refer to has
/// to call this first.
void wait_renderer_idle();
/// Destroys the renderer.
void destroy_renderer();
/// Updates the frame to render next.
void update_frame(
	const Scene_Graph& scene_graph,
	const Renderer_Settings& renderer_settings,
	const World_Settings& settings
);
/// Renders the frame.
void render_frame(const World_Settings& settings);
}  // namespace blk
