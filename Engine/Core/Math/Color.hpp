// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

/// Host-side colors.

#pragma once

#include <stdint.h>

namespace blk
{
struct Vector3;

/// A RGB color.
template <typename Type>
struct Color_RGB
{
	Type r;
	Type g;
	Type b;
};

/// A RGBA color.
template <typename Type>
struct Color_RGBA
{
	Type r;
	Type g;
	Type b;
	Type a;
};

// These pin `Color_RGBA` to a tightly packed layout with no padding between its components. `Texture::pixels` relies
// on it to be a raw pixel buffer that `Renderer/` can copy straight into a staging buffer.
static_assert(sizeof(Color_RGBA<uint8_t>) == 4);
static_assert(sizeof(Color_RGBA<float>) == 16);

/// Converts a sRGB color into a linear space.
Color_RGB<float> convert_srgb_to_linear(const Color_RGB<float>& color);
/// Converts a sRGB color into a linear space.
Vector3 convert_srgb_to_linear(const Vector3& color);
}  // namespace blk
