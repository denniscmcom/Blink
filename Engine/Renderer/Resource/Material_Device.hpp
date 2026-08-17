// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Resource/Texture_Device.hpp"

#include <vulkan/vulkan.h>

namespace blk
{
struct Context;
struct Arena;
struct Descriptor_Layouts;
struct Material;

struct Material_UBO
{
	// TODO: This is temporary until I add PBR factors.
	int dummy = 0;
};

struct Material_Device
{
	Pool_Handle<Material> handle = {};
	Texture_Device albedo = {};
	Texture_Device normal = {};
	Texture_Device orm = {};
	Buffer uniform_buffer = {};
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
};

Material_Device transfer_material(
	const Context& context,
	Arena& arena,
	const Descriptor_Layouts& descriptor_layouts,
	Pool_Handle<Material> handle
);
}  // namespace blk
