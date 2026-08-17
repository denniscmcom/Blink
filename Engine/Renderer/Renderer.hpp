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
struct Scene_Graph;

/// View data to update the frame to render.
struct Camera_View
{
	/// View point.
	Vector3 view_position = {};
	/// View matrix.
	Matrix4 view = {};
	/// Projection matrix.
	Matrix4 projection = {};
};

/// Creates renderer given a resolution.
void create_renderer(const Rect<unsigned>& rect);
/// Destroys renderer.
void destroy_renderer();
/// Updates the frame to render.
void update_frame(const Scene_Graph& scene_graph, const Camera_View& camera_view);
/// Renders the frame.
void render_frame();
}  // namespace blk
