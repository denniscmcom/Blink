// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

#include <vector>

namespace blk
{
template <typename Type>
struct Color_RGB
{
	Type r;
	Type g;
	Type b;
};

template <typename Type>
struct Color_RGBA
{
	Type r;
	Type g;
	Type b;
	Type a;
};

static_assert(sizeof(Color_RGBA<uint8_t>) == 4);
static_assert(sizeof(Color_RGBA<float>) == 16);

Color_RGB<float> convert_srgb_to_linear(const Color_RGB<float>& color);

struct Texture
{
	Resource_Metadata metadata;
	std::vector<Color_RGBA<uint8_t>> pixels;
	uint32_t width;
	uint32_t height;
};

Pool_Handle<Texture> load_texture(const char* stem);
void unload_texture(Pool_Handle<Texture> handle);
Texture* get_texture(Pool_Handle<Texture> handle);
}  // namespace blk
