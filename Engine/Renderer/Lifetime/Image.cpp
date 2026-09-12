// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Image.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Helpers.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"

#include <vulkan/vulkan.h>

blk::Result
blk::create_image(
	const Context& context,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkImageAspectFlags aspect,
	Image& image
)
{
	image = {};

	// Create Vulkan image.

	VkExtent3D extent = {};
	extent.width = width;
	extent.height = height;
	extent.depth = 1;

	VkImageCreateInfo image_create_info = {};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.format = format;
	image_create_info.extent = extent;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = tiling;
	image_create_info.usage = usage;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;

	VkImage vk_image = VK_NULL_HANDLE;

	if (vkCreateImage(context.logical_device, &image_create_info, nullptr, &vk_image) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create image\n");

		return Result::DEVICE_ERROR;
	}

	image.image = vk_image;

	// Allocate memory for the image.

	VkMemoryRequirements memory_requirements = {};
	vkGetImageMemoryRequirements(context.logical_device, vk_image, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;

	if (const Result result = find_memory_type_index(
			context,
			memory_requirements.memoryTypeBits,
			properties,
			allocate_info.memoryTypeIndex
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to find suitable memory type for image\n");
		destroy_image(context, image);

		return Result::DEVICE_ERROR;
	}

	VkDeviceMemory memory = VK_NULL_HANDLE;

	if (vkAllocateMemory(context.logical_device, &allocate_info, nullptr, &memory) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to allocate image memory\n");
		destroy_image(context, image);

		return Result::DEVICE_ERROR;
	}

	// Assign the memory before binding it, so that `destroy_image` frees it if the bind fails.
	image.memory = memory;

	// Bind the memory.

	if (vkBindImageMemory(context.logical_device, vk_image, memory, 0) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to bind memory\n");
		destroy_image(context, image);

		return Result::DEVICE_ERROR;
	}

	// Create image view.

	VkImageSubresourceRange range = {};
	range.aspectMask = aspect;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	VkImageViewCreateInfo view_create_info = {};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = vk_image;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.format = format;
	view_create_info.subresourceRange = range;

	VkImageView view = VK_NULL_HANDLE;

	if (vkCreateImageView(context.logical_device, &view_create_info, nullptr, &view) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create image view\n");
		destroy_image(context, image);

		return Result::DEVICE_ERROR;
	}

	image.view = view;
	image.format = format;

	return Result::SUCCESS;
}

void
blk::destroy_image(const Context& context, Image& image)
{
	vkDestroyImageView(context.logical_device, image.view, nullptr);
	vkDestroyImage(context.logical_device, image.image, nullptr);
	vkFreeMemory(context.logical_device, image.memory, nullptr);

	image = {};
}
