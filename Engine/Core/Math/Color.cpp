// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Core/Math/Color.hpp"

#include "Engine/Core/Math/Vector.hpp"

#include <math.h>

blk::Color_RGB<float>
blk::convert_srgb_to_linear(const Color_RGB<float>& color)
{
	// TODO (Knowledge): Read about color spaces in learnopengl.com
	return {
		.r = powf(color.r, 2.2f),
		.g = powf(color.g, 2.2f),
		.b = powf(color.b, 2.2f),
	};
}

blk::Vector3
blk::convert_srgb_to_linear(const Vector3& color)
{
	const Color_RGB srgb = {
		.r = color.x,
		.g = color.y,
		.b = color.z,
	};

	const Color_RGB<float> linear = convert_srgb_to_linear(srgb);

	return Vector3{
		.x = linear.r,
		.y = linear.g,
		.z = linear.b,
	};
}
