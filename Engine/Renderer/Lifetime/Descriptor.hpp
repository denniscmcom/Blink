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
	/// One descriptor set layout per frame in flight.
	VkDescriptorSetLayout frame_layout = VK_NULL_HANDLE;
	/// One descriptor set layout per material.
	VkDescriptorSetLayout material_layout = VK_NULL_HANDLE;
	/// A single set shared by every frame, written once at initialization.
	VkDescriptorSetLayout global_layout = VK_NULL_HANDLE;
};

/// Creates all descriptor layouts needed by the renderer.
Result create_descriptor_layouts(const Context& context, Descriptor_Layouts& layouts);
/// Destroys all renderer's descriptor layouts.
void destroy_descriptor_layouts(const Context& context, Descriptor_Layouts& layouts);
}  // namespace blk
