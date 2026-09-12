// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>

#include <stdint.h>

namespace blk
{
struct Context;
enum class Result;

/// A device image (usually a texture).
struct Image
{
	/// Vulkan image.
	VkImage image = VK_NULL_HANDLE;
	/// Vulkan image view.
	/// We cannot access a Vulkan image directly.
	VkImageView view = VK_NULL_HANDLE;
	/// Vulkan image memory.
	VkDeviceMemory memory = VK_NULL_HANDLE;
	/// Vulkan image format.
	VkFormat format;
};

/// Creates an `image`.
Result create_image(
	const Context& context,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkImageAspectFlags aspect,
	Image& image
);
/// Destroys an `image`.
// TODO (Bug): not implemented.
void destroy_image(const Context& context, Image& image);
}  // namespace blk
