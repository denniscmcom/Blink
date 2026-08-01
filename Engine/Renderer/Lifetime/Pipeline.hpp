// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>

#include <span>

namespace blk
{
struct Context;

struct Pipeline
{
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

VkShaderModule create_shader_module(const Context& context, const char* stem);

Pipeline create_pipeline(
	const Context& context,
	VkSurfaceFormatKHR surface_format,
	VkFormat depth_format,
	std::span<const VkDescriptorSetLayout> descriptor_layouts,
	std::span<const VkPipelineShaderStageCreateInfo> shader_stages,
	std::span<const VkVertexInputBindingDescription> vertex_binding_descriptions,
	std::span<const VkVertexInputAttributeDescription> vertex_attribute_descriptions,
	std::span<const VkPushConstantRange> push_constant_ranges
);
}  // namespace blk
