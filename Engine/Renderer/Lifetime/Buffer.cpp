// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Buffer.hpp"

#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Helpers.hpp"
#include "Engine/Renderer/Lifetime/Command.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"

blk::Result
blk::create_buffer(
	const Context& context,
	const VkDeviceSize size,
	const VkBufferUsageFlags usage,
	const VkMemoryPropertyFlags properties,
	Buffer& buffer
)
{
	buffer = {};

	// Create Vulkan buffer.

	VkBufferCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.size = size;
	create_info.usage = usage;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer vk_buffer = VK_NULL_HANDLE;

	if (vkCreateBuffer(context.logical_device, &create_info, nullptr, &vk_buffer) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create buffer\n");

		return Result::DEVICE_ERROR;
	}

	buffer.buffer = vk_buffer;

	// Allocate memory for it.

	VkMemoryRequirements memory_requirements = {};
	vkGetBufferMemoryRequirements(context.logical_device, vk_buffer, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;

	if (const Result result = find_memory_type_index(
			context,
			memory_requirements.memoryTypeBits,
			properties,
			memory_allocate_info.memoryTypeIndex
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to find suitable buffer memory\n");
		destroy_buffer(context, buffer);

		return result;
	}

	VkDeviceMemory memory = VK_NULL_HANDLE;

	if (vkAllocateMemory(context.logical_device, &memory_allocate_info, nullptr, &memory) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to allocate memory for buffer\n");
		destroy_buffer(context, buffer);

		return Result::DEVICE_ERROR;
	}

	buffer.memory = memory;
	buffer.size = size;

	// Bind the buffer memory.

	if (vkBindBufferMemory(context.logical_device, vk_buffer, memory, 0) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to bind memory to buffer\n");
		destroy_buffer(context, buffer);

		return Result::DEVICE_ERROR;
	}

	buffer.map = nullptr;

	return Result::SUCCESS;
}

blk::Result
blk::create_host_device_buffer(
	const Context& context,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	Host_Device_Buffer& buffer
)
{
	// Default initialize buffer.
	buffer = {};

	// Create host buffer.
	BLK_SUCCESS_OR_RETURN(create_buffer(
		context,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		buffer.host
	));

	// Create device buffer.
	if (const Result result = create_buffer(
			context,
			size,
			usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			buffer.device
		);
		result != Result::SUCCESS)
	{
		// Clean up previous resources.
		destroy_buffer(context, buffer.host);

		return result;
	}

	return Result::SUCCESS;
}

blk::Result
blk::update_buffer(const Buffer& buffer, const void* data, const VkDeviceSize size, const VkDeviceSize offset)
{
	if (!BLK_VERIFY(data))
	{
		return Result::INVALID_ARGUMENTS;
	}

	if (!buffer.map)
	{
		BLK_ERROR("Buffer is not mapped\n");

		return Result::INVALID_ARGUMENTS;
	}

	// Copy `data` to `buffer.map`.
	memcpy(static_cast<char*>(buffer.map) + offset, data, size);

	return Result::SUCCESS;
}

blk::Result
blk::map_buffer(const Context& context, Buffer& buffer)
{
	if (vkMapMemory(context.logical_device, buffer.memory, 0, buffer.size, 0, &buffer.map) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to map memory buffer\n");

		return Result::DEVICE_ERROR;
	}

	return Result::SUCCESS;
}

void
blk::unmap_buffer(const Context& context, Buffer& buffer)
{
	// Unmap memory first.
	vkUnmapMemory(context.logical_device, buffer.memory);

	// Do not forget to clean up dangling pointer.
	buffer.map = nullptr;
}

blk::Result
blk::copy_buffer(const Context& context, const Buffer& src, const Buffer& dst)
{
	// Both buffers must be the same size.
	if (!BLK_VERIFY(src.size == dst.size))
	{
		return Result::INVALID_ARGUMENTS;
	}

	// Create a command buffer from the transient command pool.
	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	BLK_SUCCESS_OR_RETURN(create_command_buffer(context, context.transient_command_pool, command_buffer));

	begin_one_time_commands(command_buffer);

	// Record copy command.
	VkBufferCopy buffer_copy = {};
	buffer_copy.size = dst.size;
	vkCmdCopyBuffer(command_buffer, src.buffer, dst.buffer, 1, &buffer_copy);

	end_one_time_commands(context, command_buffer);

	destroy_command_buffer(context, context.transient_command_pool, command_buffer);

	return Result::SUCCESS;
}

blk::Result
blk::copy_buffer_to_image(
	const Context& context,
	const Buffer& src,
	VkImage dst,
	const uint32_t width,
	const uint32_t height
)
{
	// Create command buffer from the transient pool.
	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	BLK_SUCCESS_OR_RETURN(create_command_buffer(context, context.transient_command_pool, command_buffer));

	begin_one_time_commands(command_buffer);

	// `dst` layout starts as `UNDEFINED` and we transition it to `TRANSFER_DST_OPTIMAL`.
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

	// Copy the bufffer into the image.

	VkBufferImageCopy region = {};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = {.width = width, .height = height, .depth = 1};

	vkCmdCopyBufferToImage(command_buffer, src.buffer, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// Transition the image for shader sampling.
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

	destroy_command_buffer(context, context.transient_command_pool, command_buffer);

	return Result::SUCCESS;
}

void
blk::destroy_buffer(const Context& context, Buffer& buffer)
{
	if (buffer.map)
	{
		unmap_buffer(context, buffer);
	}

	vkDestroyBuffer(context.logical_device, buffer.buffer, nullptr);
	vkFreeMemory(context.logical_device, buffer.memory, nullptr);

	buffer = {};
}
