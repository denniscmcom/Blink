// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>

#include <stdint.h>

namespace blk
{
struct Context;
struct Vertex_UV;
template <typename Type, size_t Capacity>
struct Array;
enum class Result;

/// Finds which device memory type satisfies both what the resource needs and what the caller requested.
/// @param memory_type_bits Memory type needed by the resource (buffer, image, etc.).
/// @param memory_property_flags Any memory property needed by the caller.
Result find_memory_type_index(
	const Context& context,
	uint32_t memory_type_bits,
	VkMemoryPropertyFlags memory_property_flags,
	uint32_t& device_memory_index
);

/// Thin wrapper around a pipeline barrier.
/// Transitions an image from one state into another.
void transition_image_layout(
	VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout,
	VkImageLayout new_layout,
	VkAccessFlags2 src_access,
	VkAccessFlags2 dst_access,
	VkPipelineStageFlags2 src_stage,
	VkPipelineStageFlags2 dst_stage,
	VkImageAspectFlags image_aspect
);

/// Thin wrapper around beginning a `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT` command buffer.
Result begin_one_time_commands(VkCommandBuffer command_buffer);
/// Thin wrapper around ending a `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`, command buffer, submitting it and
/// waiting for completion.
Result end_one_time_commands(const Context& context, VkCommandBuffer command_buffer);

/// Thin wrapper to create a `VkPipelineShaderStageCreateInfo`.
VkPipelineShaderStageCreateInfo get_pipeline_shader_stage_create_info(
	VkShaderModule module,
	VkShaderStageFlagBits stage,
	const char* entry_name
);

/// Gets vertex input binding descriptions for `Vertex_UV`.
Array<VkVertexInputBindingDescription, 1> get_vertex_uv_input_binding_descriptions();
/// Gets vertex input attribute descriptions for `Vertex_UV`.
Array<VkVertexInputAttributeDescription, 5> get_vertex_uv_input_attribute_descriptions();
}  // namespace blk
