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

// Layout must match Shaders/Shared.slang (std140).
struct Material_UBO
{
	float shininess;
};

struct Material_Device
{
	Pool_Handle<Material> handle;
	Texture_Device diffuse_map;
	Texture_Device specular_map;
	Buffer uniform_buffer;
	VkDescriptorSet descriptor_set;
};

Material_Device transfer_material(
	const Context& context,
	Arena& arena,
	const Descriptor_Layouts& descriptor_layouts,
	Pool_Handle<Material> handle
);
}  // namespace blk
