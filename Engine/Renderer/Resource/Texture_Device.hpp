// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Renderer/Lifetime/Image.hpp"

#include <vulkan/vulkan.h>

namespace blk
{
struct Context;
struct Arena;
struct Texture;

/// A device texture.
struct Texture_Device
{
	/// Host texture handle.
	Pool_Handle<Texture> host_handle;
	/// Device image backing the texture.
	Image image;
};

/// Transfer a host `Texture` into device.
///
/// It checks for duplicated textures before transferring. If a texture already exists in device, it reuses it.
Result transfer_texture(
	const Context& context,
	Arena& arena,
	Pool_Handle<Texture> host_handle,
	VkFormat format,
	Texture_Device& texture
);
/// Unloads a `texture` from device.
// TODO (Bug): not implemented.
void unload_texture_from_device(const Context& context, Arena& arena, Texture_Device& texture);
}  // namespace blk
