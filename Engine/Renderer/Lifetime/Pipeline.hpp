// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>

namespace blk
{
struct Context;
template <typename Type>
struct Array_View;
enum class Result;

/// A graphics pipeline.
struct Pipeline
{
	/// Vulkan pipeline.
	VkPipeline pipeline;
	/// Vulkan pipeline layout.
	VkPipelineLayout layout;
};

/// Creates a Vulkan shader module.
/// @param stem A null-terminated string. It should be the filename without extension of a shader file.
Result create_shader_module(const Context& context, const char* stem, VkShaderModule& module);
/// Destroys a Vulkan shader module.
void destroy_shader_module(const Context& context, VkShaderModule module);
/// Creates a `pipeline`.
Result create_pipeline(
	const Context& context,
	VkSurfaceFormatKHR surface_format,
	VkFormat depth_format,
	const Array_View<const VkDescriptorSetLayout>& descriptor_layouts,
	const Array_View<const VkPipelineShaderStageCreateInfo>& shader_stages,
	const Array_View<const VkVertexInputBindingDescription>& vertex_binding_descriptions,
	const Array_View<const VkVertexInputAttributeDescription>& vertex_attribute_descriptions,
	const Array_View<const VkPushConstantRange>& push_constant_ranges,
	Pipeline& pipeline
);
/// Destroys a `pipeline`.
// TODO (Bug): not implemented.
void destroy_pipeline(const Context& context, Pipeline& pipeline);
}  // namespace blk
