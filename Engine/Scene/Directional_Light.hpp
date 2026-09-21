// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Color.hpp"

namespace blk
{
/// Represents a light that gets emitted in a specific direction.
struct Directional_Light
{
	/// Light color. The default value is full white.
	Color_RGB<float> color = {.r = 1.0f, .g = 1.0f, .b = 1.0f};
	/// Only one `Directional_Light` in the scene can represent the sun.
	///
	/// If it is true, the `x` scale component of the node is used as the sun radius in degress.
	bool is_sun;
};
}  // namespace blk
