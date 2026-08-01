// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>

#include <stdint.h>
#include <vector>

namespace blk
{
struct Context;
struct Vertex_PNT;
struct Vertex_PNC;

uint32_t find_memory_type_index(
	const Context& context,
	uint32_t memory_type_bits,
	VkMemoryPropertyFlags memory_property_flags
);

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

void begin_one_time_commands(VkCommandBuffer command_buffer);
void end_one_time_commands(const Context& context, VkCommandBuffer command_buffer);

VkPipelineShaderStageCreateInfo get_pipeline_shader_stage_create_info(
	VkShaderModule module,
	VkShaderStageFlagBits stage,
	const char* entry_name
);

template <typename Type>
std::vector<VkVertexInputBindingDescription> get_vertex_input_descriptions();
template <typename Type>
std::vector<VkVertexInputAttributeDescription> get_vertex_input_attribute_descriptions();
template <>
std::vector<VkVertexInputAttributeDescription> get_vertex_input_attribute_descriptions<Vertex_PNT>();
template <>
std::vector<VkVertexInputAttributeDescription> get_vertex_input_attribute_descriptions<Vertex_PNC>();

VkCommandBuffer create_command_buffer(const Context& context, VkCommandPool pool);
VkSemaphore create_semaphore(const Context& context);
VkFence create_fence(const Context& context);

template <typename Type>
std::vector<VkVertexInputBindingDescription>
get_vertex_input_descriptions()
{
	std::vector<VkVertexInputBindingDescription> descriptions = {
		{.binding = 0, .stride = sizeof(Type), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}
	};

	return descriptions;
}

template <typename Type>
std::vector<VkVertexInputAttributeDescription>
get_vertex_input_attribute_descriptions()
{
	static_assert(sizeof(Type) == 0, "No vertex attribute description defined for this type");

	return {};
}
}  // namespace blk
