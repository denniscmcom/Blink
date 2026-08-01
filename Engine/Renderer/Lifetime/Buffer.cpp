// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Buffer.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Renderer/Helpers.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"

blk::Buffer
blk::create_buffer(
	const Context& context,
	const VkDeviceSize size,
	const VkBufferUsageFlags usage,
	const VkMemoryPropertyFlags properties
)
{
	VkBufferCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.size = size;
	create_info.usage = usage;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;

	if (vkCreateBuffer(context.logical_device, &create_info, nullptr, &buffer) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create buffer\n");
	}

	VkMemoryRequirements memory_requirements = {};
	vkGetBufferMemoryRequirements(context.logical_device, buffer, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex =
		find_memory_type_index(context, memory_requirements.memoryTypeBits, properties);

	VkDeviceMemory memory;

	if (vkAllocateMemory(context.logical_device, &memory_allocate_info, nullptr, &memory) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to allocate memory for buffer\n");
	}

	if (vkBindBufferMemory(context.logical_device, buffer, memory, 0) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to bind memory to buffer\n");
	}

	return Buffer{.buffer = buffer, .memory = memory, .size = size, .map = nullptr};
}

blk::Host_Device_Buffer
blk::create_host_device_buffer(const Context& context, VkDeviceSize size, VkBufferUsageFlags usage)
{
	const Buffer host = create_buffer(
		context,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	const Buffer device = create_buffer(
		context,
		size,
		usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	return Host_Device_Buffer{.host = host, .device = device};
}

void
blk::update_buffer(const Buffer& buffer, const void* data, const VkDeviceSize size, const VkDeviceSize offset)
{
	if (!buffer.map)
	{
		BLK_FATAL("Buffer is not mapped\n");
	}

	memcpy(static_cast<char*>(buffer.map) + offset, data, size);
}

void
blk::map_buffer(const Context& context, Buffer& buffer)
{
	if (vkMapMemory(context.logical_device, buffer.memory, 0, buffer.size, NULL, &buffer.map) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to map memory buffer\n");
	}
}

void
blk::unmap_buffer(const Context& context, Buffer& buffer)
{
	vkUnmapMemory(context.logical_device, buffer.memory);
	buffer.map = nullptr;
}

void
blk::copy_buffer(const Context& context, const Buffer& src, const Buffer& dst)
{
	if (src.size != dst.size)
	{
		BLK_FATAL("Buffer are not the same size\n");
	}

	VkCommandBuffer command_buffer = create_command_buffer(context, context.transient_command_pool);
	begin_one_time_commands(command_buffer);

	VkBufferCopy buffer_copy = {};
	buffer_copy.size = dst.size;
	vkCmdCopyBuffer(command_buffer, src.buffer, dst.buffer, 1, &buffer_copy);

	end_one_time_commands(context, command_buffer);
}

void
blk::copy_buffer_to_image(
	const Context& context,
	const Buffer& src,
	VkImage dst,
	const uint32_t width,
	const uint32_t height
)
{
	VkCommandBuffer command_buffer = create_command_buffer(context, context.transient_command_pool);
	begin_one_time_commands(command_buffer);

	transition_image_layout(
		command_buffer,
		dst,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		{},
		VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	VkBufferImageCopy region = {};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = {.width = width, .height = height, .depth = 1};

	vkCmdCopyBufferToImage(command_buffer, src.buffer, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	transition_image_layout(
		command_buffer,
		dst,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_ACCESS_2_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	end_one_time_commands(context, command_buffer);
}

void
blk::destroy_buffer(const Context& context, const Buffer& buffer)
{
	vkDestroyBuffer(context.logical_device, buffer.buffer, nullptr);
}
