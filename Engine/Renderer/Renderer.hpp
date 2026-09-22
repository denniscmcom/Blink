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
struct Camera_View
{
	/// View point.
	Vector3 view_position;
	/// View matrix.
	Matrix4 view;
	/// Projection matrix.
	Matrix4 projection;
};

/// Creates the renderer.
/// @param rect The renderer resolution.
Result create_renderer(const Rect<unsigned>& rect);
/// Precomputes the renderer's static lookup tables.
Result bake_renderer();
/// Waits until the device has finished every submitted command.
///
/// `render_frame` returns as soon as the frame is submitted, so the GPU is still reading the resources of the last
/// `MAX_FRAMES_IN_FLIGHT` frames after the main loop exits. Anything that destroys a resource those frames refer to has
/// to call this first.
void wait_renderer_idle();
/// Destroys the renderer.
void destroy_renderer();
/// Updates the frame to render next.
void update_frame(const Scene_Graph& scene_graph, const Camera_View& camera_view, const World_Settings& settings);
/// Renders the frame.
void render_frame(const World_Settings& settings);
}  // namespace blk
