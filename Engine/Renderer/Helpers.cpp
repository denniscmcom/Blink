// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Helpers.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Resource/Mesh.hpp"

#include <vulkan/vulkan.h>

#include <stdint.h>

blk::Result
blk::find_memory_type_index(
	const Context& context,
	const uint32_t memory_type_bits,
	const VkMemoryPropertyFlags memory_property_flags,
	uint32_t& device_memory_index
)
{
	VkPhysicalDeviceMemoryProperties2 physical_device_memory_properties = {};
	physical_device_memory_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

	vkGetPhysicalDeviceMemoryProperties2(context.physical_device, &physical_device_memory_properties);

	bool found_physical_device_memory_type = false;

	for (uint32_t i = 0; i < physical_device_memory_properties.memoryProperties.memoryTypeCount; i++)
	{
		const uint32_t memory_type_filter = memory_type_bits & (1 << i);
		const VkMemoryPropertyFlags memory_property_flags_filter =
			physical_device_memory_properties.memoryProperties.memoryTypes[i].propertyFlags & memory_property_flags;

		if (memory_type_filter && memory_property_flags_filter == memory_property_flags)
		{
			device_memory_index = i;
			found_physical_device_memory_type = true;
			break;
		}
	}

	if (!found_physical_device_memory_type)
	{
		BLK_ERROR("Failed to find suitable physical device memory type\n");

		return Result::DEVICE_ERROR;
	}

	return Result::SUCCESS;
}

void
blk::transition_image_layout(
	VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout,
	VkImageLayout new_layout,
	VkAccessFlags2 src_access,
	VkAccessFlags2 dst_access,
	VkPipelineStageFlags2 src_stage,
	VkPipelineStageFlags2 dst_stage,
	VkImageAspectFlags image_aspect
)
{
	VkImageSubresourceRange range = {};
	range.aspectMask = image_aspect;
	range.levelCount = 1;
	range.layerCount = 1;

	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = image;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcAccessMask = src_access;
	barrier.dstAccessMask = dst_access;
	barrier.srcStageMask = src_stage;
	barrier.dstStageMask = dst_stage;
	barrier.subresourceRange = range;

	VkDependencyInfo dependency_info = {};
	dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency_info.imageMemoryBarrierCount = 1;
	dependency_info.pImageMemoryBarriers = &barrier;

	vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

blk::Result
blk::begin_one_time_commands(VkCommandBuffer command_buffer)
{
	VkCommandBufferBeginInfo command_buffer_begin_info = {};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to begin one time command buffer\n");

		return Result::DEVICE_ERROR;
	}

	return Result::SUCCESS;
}

blk::Result
blk::end_one_time_commands(const Context& context, VkCommandBuffer command_buffer)
{
	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to end one time command buffer\n");

		return Result::DEVICE_ERROR;
	}

	VkCommandBufferSubmitInfo command_buffer_submit_info = {};
	command_buffer_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	command_buffer_submit_info.commandBuffer = command_buffer;

	VkSubmitInfo2 submit_info_2 = {};
	submit_info_2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info_2.commandBufferInfoCount = 1;
	submit_info_2.pCommandBufferInfos = &command_buffer_submit_info;

	if (vkQueueSubmit2(context.queue, 1, &submit_info_2, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to submit one time command buffer\n");

		return Result::DEVICE_ERROR;
	}

	if (vkQueueWaitIdle(context.queue) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to wait for queue\n");

		return Result::DEVICE_ERROR;
	}

	return Result::SUCCESS;
}

VkPipelineShaderStageCreateInfo
blk::get_pipeline_shader_stage_create_info(VkShaderModule module, VkShaderStageFlagBits stage, const char* entry_name)
{
	VkPipelineShaderStageCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	create_info.stage = stage;
	create_info.module = module;
	create_info.pName = entry_name;

	return create_info;
}

blk::Array<VkVertexInputBindingDescription, 1>
blk::get_vertex_uv_input_binding_descriptions()
{
	return {{
		{.binding = 0, .stride = sizeof(Vertex_UV), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
	}};
}

blk::Array<VkVertexInputAttributeDescription, 5>
blk::get_vertex_uv_input_attribute_descriptions()
{
	return {{
		{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex_UV, position)},
		{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex_UV, normal)},
		{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex_UV, tangent)},
		{.location = 3, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex_UV, bitangent)},
		{.location = 4, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex_UV, texture_coord)},
	}};
}
