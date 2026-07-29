#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace blk
{
struct Context;

struct Swapchain
{
	VkSwapchainKHR swapchain;
	std::vector<VkImage> images;
	std::vector<VkImageView> image_views;
	VkSurfaceFormatKHR surface_format;
	VkExtent2D extent;
	float aspect_ratio;
	std::vector<VkSemaphore> render_finished_semaphores;
};

Swapchain create_swapchain(const Context& context);
}  // namespace blk
