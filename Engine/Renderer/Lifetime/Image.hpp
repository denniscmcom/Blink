#pragma once

#include <vulkan/vulkan.h>

#include <stdint.h>

namespace blk
{
struct Context;

struct Image
{
	VkImage image = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkFormat format;
};

Image create_image(
	const Context& context,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkImageAspectFlags aspect
);
}  // namespace blk
