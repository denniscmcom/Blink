// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>

namespace blk
{
struct Context;

struct Buffer
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize size;
	void* map;
};

struct Host_Device_Buffer
{
	Buffer host;
	Buffer device;
};

Buffer create_buffer(
	const Context& context,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties
);

Host_Device_Buffer create_host_device_buffer(const Context& context, VkDeviceSize size, VkBufferUsageFlags usage);
void update_buffer(const Buffer& buffer, const void* data, VkDeviceSize size, VkDeviceSize offset);
void map_buffer(const Context& context, Buffer& buffer);
void unmap_buffer(const Context& context, Buffer& buffer);
void copy_buffer(const Context& context, const Buffer& src, const Buffer& dst);
void copy_buffer_to_image(const Context& context, const Buffer& src, VkImage dst, uint32_t width, uint32_t height);
void destroy_buffer(const Context& context, const Buffer& buffer);
}  // namespace blk
