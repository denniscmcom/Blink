// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Renderer/Lifetime/Image.hpp"

#include <vulkan/vulkan.h>

#include <optional>

namespace blk
{
struct Context;
struct Arena;
struct Texture;

struct Texture_Device
{
	Pool_Handle<Texture> handle = {};
	Image image = {};
};

/// Transfers texture to GPU device.
std::optional<Texture_Device> transfer_texture(
	const Context& context,
	Arena& arena,
	const Pool_Handle<Texture>& handle,
	VkFormat format
);
}  // namespace blk
