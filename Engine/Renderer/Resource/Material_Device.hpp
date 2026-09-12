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
enum class Result;

/// Material uniform buffer object.
struct Material_UBO
{
	// TODO (Feature): This is temporary until I add PBR factors.
	int dummy;
};

/// A device material.
struct Material_Device
{
	/// Host material handle.
	Pool_Handle<Material> host_handle;
	/// Device albedo texture.
	Texture_Device albedo;
	/// Device normal texture.
	Texture_Device normal;
	/// Device ORM texture.
	Texture_Device orm;
	/// Material uniform buffer.
	Buffer uniform_buffer;
	/// Material descriptor set.
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
};

/// Transfer a host `Material` into device.
///
/// It checks for duplicated materials before transferring. If a material already exists in device, it reuses it.
Result transfer_material(
	const Context& context,
	const Descriptor_Layouts& descriptor_layouts,
	Arena& arena,
	Pool_Handle<Material> host_handle,
	Material_Device& material
);
/// Unloads a `material` from device.
// TODO (Bug): not implemented.
void unload_material_from_device(const Context& context, Arena& arena, Material_Device& material);
}  // namespace blk
