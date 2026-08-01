// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Vector.hpp"

namespace blk
{
struct Scene_Graph;

struct Camera_View
{
	Vector3 view_position;
	Matrix4 view;
	Matrix4 projection;
};

void create_renderer(unsigned width, unsigned height);
void destroy_renderer();
void update_frame(const Scene_Graph& scene_graph, const Camera_View& camera_view);
void render_frame();
}  // namespace blk
