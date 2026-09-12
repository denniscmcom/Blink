// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>

namespace blk
{
struct Context;
enum class Result;

/// A device buffer.
struct Buffer
{
	/// Vulkan buffer.
	VkBuffer buffer = VK_NULL_HANDLE;
	/// Vulkan buffer memory.
	VkDeviceMemory memory = VK_NULL_HANDLE;
	/// Vulkan buffer size.
	VkDeviceSize size;
	/// A pointer to a memory section that maps host-device memory.
	/// It can be nullptr if `buffer` is not mapped. @see `map_buffer`.
	void* map;
};

/// A staging buffer pair for uploading data to device-local memory.
/// Used for static or infrequently-updated data (vertex buffer, index buffers, etc.)
/// The host buffer is mapped and written to by the host, then copied to the device buffer via `vkCmdCopyBuffer`.
/// @see `copy_buffer`.
struct Host_Device_Buffer
{
	/// Host-visible staging buffer (transfer source).
	Buffer host;
	/// Device-local buffer used by the GPU during rendering (transfer destination).
	Buffer device;
};

/// Creates `buffer`.
Result create_buffer(
	const Context& context,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	Buffer& buffer
);
/// Creates a staging buffer pair.
/// @param size Both buffers have the same size.
/// @param usage The usage for the device buffer.
Result create_host_device_buffer(
	const Context& context,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	Host_Device_Buffer& buffer
);
/// Updates a `buffer` with new `data`.
/// @warning `Buffer` should be mapped. @see `map_buffer`.
Result update_buffer(const Buffer& buffer, const void* data, VkDeviceSize size, VkDeviceSize offset);
/// Maps a device buffer to host memory, so that it can be written to.
/// @see `Buffer::map`.
Result map_buffer(const Context& context, Buffer& buffer);
/// Unmaps a `buffer` memory.
void unmap_buffer(const Context& context, Buffer& buffer);
/// Copies data from `src` to `dst`.
/// @param src Should be created with the usage flag `VK_BUFFER_USAGE_TRANSFER_SRC_BIT`.
/// @param dst Should be created with the usage flag `VK_BUFFER_USAGE_TRANSFER_DST_BIT`.
/// @warning Both buffers must be the same size.
/// @see `Host_Device_Buffer`.
Result copy_buffer(const Context& context, const Buffer& src, const Buffer& dst);
/// Uploads pixel data from host buffer into a device image (typically a texture).
Result copy_buffer_to_image(const Context& context, const Buffer& src, VkImage dst, uint32_t width, uint32_t height);
/// Destroys `buffer`. It unmaps it first if it is mapped.
void destroy_buffer(const Context& context, Buffer& buffer);
}  // namespace blk
