#pragma once

#include <vulkan/vulkan.h>

#include <windows.h>

namespace blk
{
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct Context
{
	uint32_t api_version;
	VkInstance instance = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice logical_device = VK_NULL_HANDLE;
	VkQueue graphics_queue = VK_NULL_HANDLE;
	uint32_t graphics_queue_family_index = ~0;

	VkCommandPool frame_command_pool = VK_NULL_HANDLE;
	VkCommandPool transient_command_pool = VK_NULL_HANDLE;

	VkSampler sampler = VK_NULL_HANDLE;
};

Context create_context(HWND window);
}  // namespace blk
