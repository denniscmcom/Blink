// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Color.hpp"

namespace blk
{
/// Node point light data. It represents a point light node in `Scene_Graph`.
struct Point_Light
{
	/// Light color. The default value is full white.
	Color_RGB<float> color = {.r = 1.0f, .g = 1.0f, .b = 1.0f};
};
}  // namespace blk
