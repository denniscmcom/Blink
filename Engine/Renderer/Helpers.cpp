// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Helpers.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Resource/Mesh.hpp"

#include <vulkan/vulkan.h>

#include <stdint.h>

uint32_t
blk::find_memory_type_index(
	const Context& context,
	const uint32_t memory_type_bits,
	const VkMemoryPropertyFlags memory_property_flags
)
{
	VkPhysicalDeviceMemoryProperties2 physical_device_memory_properties = {};
	physical_device_memory_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

	vkGetPhysicalDeviceMemoryProperties2(context.physical_device, &physical_device_memory_properties);

	uint32_t selected_physical_device_memory_type_index = ~0;

	for (uint32_t i = 0; i < physical_device_memory_properties.memoryProperties.memoryTypeCount; i++)
	{
		const uint32_t memory_type_filter = memory_type_bits & 1 << i;
		const VkMemoryPropertyFlags memory_property_flags_filter =
			physical_device_memory_properties.memoryProperties.memoryTypes[i].propertyFlags & memory_property_flags;

		if (memory_type_filter && memory_property_flags_filter == memory_property_flags)
		{
			selected_physical_device_memory_type_index = i;
			break;
		}
	}

	if (selected_physical_device_memory_type_index == ~0)
	{
		BLK_FATAL("Failed to find suitable physical device memory type\n");
	}

	return selected_physical_device_memory_type_index;
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

void
blk::begin_one_time_commands(VkCommandBuffer command_buffer)
{
	VkCommandBufferBeginInfo command_buffer_begin_info = {};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to begin command buffer\n");
	}
}

void
blk::end_one_time_commands(const Context& context, VkCommandBuffer command_buffer)
{
	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to end command buffer\n");
	}

	VkCommandBufferSubmitInfo command_buffer_submit_info = {};
	command_buffer_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	command_buffer_submit_info.commandBuffer = command_buffer;

	VkSubmitInfo2 submit_info_2 = {};
	submit_info_2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info_2.commandBufferInfoCount = 1;
	submit_info_2.pCommandBufferInfos = &command_buffer_submit_info;

	if (vkQueueSubmit2(context.graphics_queue, 1, &submit_info_2, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to submit command buffer\n");
	}

	if (vkQueueWaitIdle(context.graphics_queue) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to wait for queue\n");
	}
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

template <>
std::vector<VkVertexInputAttributeDescription>
blk::get_vertex_input_attribute_descriptions<blk::Vertex_PNT>()
{
	std::vector<VkVertexInputAttributeDescription> descriptions = {
		{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex_PNT, position)},
		{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex_PNT, normal)},
		{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex_PNT, texture_coord)},
	};

	return descriptions;
}

template <>
std::vector<VkVertexInputAttributeDescription>
blk::get_vertex_input_attribute_descriptions<blk::Vertex_PNC>()
{
	std::vector<VkVertexInputAttributeDescription> descriptions = {
		{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex_PNC, position)},
		{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex_PNC, normal)},
		{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex_PNC, color)},
	};

	return descriptions;
}

VkCommandBuffer
blk::create_command_buffer(const Context& context, VkCommandPool pool)
{
	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = pool;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;

	if (vkAllocateCommandBuffers(context.logical_device, &command_buffer_allocate_info, &command_buffer) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to allocate command buffers\n");
	}

	return command_buffer;
}

VkSemaphore
blk::create_semaphore(const Context& context)
{
	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore;

	if (vkCreateSemaphore(context.logical_device, &semaphore_create_info, nullptr, &semaphore) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create render finished semaphore\n");
	}

	return semaphore;
}

VkFence
blk::create_fence(const Context& context)
{
	VkFenceCreateInfo fence_create_info = {};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence fence;

	if (vkCreateFence(context.logical_device, &fence_create_info, nullptr, &fence) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create in-flight fence\n");
	}

	return fence;
}
