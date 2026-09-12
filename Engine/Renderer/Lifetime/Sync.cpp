// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Sync.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"

blk::Result
blk::create_semaphore(const Context& context, VkSemaphore& semaphore)
{
	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(context.logical_device, &semaphore_create_info, nullptr, &semaphore) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create semaphore\n");

		return Result::DEVICE_ERROR;
	}

	return Result::SUCCESS;
}

blk::Result
blk::create_fence(const Context& context, VkFence& fence)
{
	VkFenceCreateInfo fence_create_info = {};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(context.logical_device, &fence_create_info, nullptr, &fence) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create fence\n");

		return Result::DEVICE_ERROR;
	}

	return Result::SUCCESS;
}

void
blk::destroy_semaphore(const Context& context, VkSemaphore& semaphore)
{
	vkDestroySemaphore(context.logical_device, semaphore, nullptr);

	semaphore = VK_NULL_HANDLE;
}

void
blk::destroy_fence(const Context& context, VkFence& fence)
{
	vkDestroyFence(context.logical_device, fence, nullptr);

	fence = VK_NULL_HANDLE;
}
