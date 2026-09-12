// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>

namespace blk
{
struct Context;
enum class Result;

/// Renderer's descriptor layouts.
/// A descriptor layouts defines the shape of data a shader expects to receive.
struct Descriptor_Layouts
{
	/// Descriptor pool from which descriptor set layouts are allocated.
	VkDescriptorPool pool = VK_NULL_HANDLE;
	/// Descriptor layout for camera data.
	VkDescriptorSetLayout camera_layout = VK_NULL_HANDLE;
	/// Descriptor layout for light data.
	VkDescriptorSetLayout light_layout = VK_NULL_HANDLE;
	/// Descriptor layout for PBR textured material data.
	/// @see `Material`.
	VkDescriptorSetLayout material_layout = VK_NULL_HANDLE;
};

/// Creates all descriptor layouts needed by the renderer.
Result create_descriptor_layouts(const Context& context, Descriptor_Layouts& layouts);
/// Destroys all renderer's descriptor layouts.
// TODO (Bug): Implementation missing.
void destroy_descriptor_layouts(const Context& context, Descriptor_Layouts& layouts);
}  // namespace blk
