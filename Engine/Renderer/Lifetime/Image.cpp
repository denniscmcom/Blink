#include "Renderer/Lifetime/Image.hpp"

#include "Platform/Log.hpp"
#include "Renderer/Helpers.hpp"
#include "Renderer/Lifetime/Context.hpp"

#include <vulkan/vulkan.h>

blk::Image
blk::create_image(
	const Context& context,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkImageAspectFlags aspect
)
{
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

	VkImage image;

	if (vkCreateImage(context.logical_device, &image_create_info, nullptr, &image) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create image\n");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(context.logical_device, image, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = find_memory_type_index(context, memory_requirements.memoryTypeBits, properties);

	VkDeviceMemory memory;

	if (vkAllocateMemory(context.logical_device, &allocate_info, nullptr, &memory) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to allocate memory\n");
	}

	if (vkBindImageMemory(context.logical_device, image, memory, 0) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to bind memory\n");
	}

	VkImageSubresourceRange range = {};
	range.aspectMask = aspect;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	VkImageViewCreateInfo view_create_info = {};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = image;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.format = format;
	view_create_info.subresourceRange = range;

	VkImageView view;

	if (vkCreateImageView(context.logical_device, &view_create_info, nullptr, &view) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create image view\n");
	}

	return Image{.image = image, .view = view, .memory = memory, .format = format};
}
