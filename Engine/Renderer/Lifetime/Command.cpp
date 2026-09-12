// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Command.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"

blk::Result
blk::create_command_buffer(const Context& context, VkCommandPool pool, VkCommandBuffer& buffer)
{
	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = pool;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(context.logical_device, &command_buffer_allocate_info, &buffer) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to allocate command buffers\n");

		return Result::DEVICE_ERROR;
	}

	return Result::SUCCESS;
}

void
blk::destroy_command_buffer(const Context& context, VkCommandPool pool, VkCommandBuffer& buffer)
{
	vkFreeCommandBuffers(context.logical_device, pool, 1, &buffer);

	buffer = VK_NULL_HANDLE;
}
