// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Swapchain.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Renderer/Helpers.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"

#include <vulkan/vulkan.h>

#include <vector>

blk::Swapchain
blk::create_swapchain(const Context& context)
{
	BLK_DEBUG("Creating swapchain...\n");
	Swapchain swapchain = {};

	BLK_DEBUG("Getting surface capabilities\n");

	VkSurfaceCapabilitiesKHR surface_capabilities;

	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physical_device, context.surface, &surface_capabilities) !=
		VK_SUCCESS)
	{
		BLK_FATAL("Failed to get surface capabilities\n");
	}

	BLK_DEBUG("Getting surface formats...\n");
	uint32_t surface_format_count = 0;

	if (vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device, context.surface, &surface_format_count, nullptr))
	{
		BLK_FATAL("Failed to get surface format count\n");
	}

	BLK_DEBUG("Got %u surface format(s)\n", surface_format_count);

	if (surface_format_count == 0)
	{
		BLK_FATAL("Surface formats not found\n");
	}

	std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);

	if (vkGetPhysicalDeviceSurfaceFormatsKHR(
			context.physical_device,
			context.surface,
			&surface_format_count,
			surface_formats.data()
		))
	{
		BLK_FATAL("Failed to get surface formats\n");
	}

	BLK_DEBUG("Getting surface present modes...\n");
	uint32_t surface_present_mode_count = 0;

	if (vkGetPhysicalDeviceSurfacePresentModesKHR(
			context.physical_device,
			context.surface,
			&surface_present_mode_count,
			nullptr
		) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to get surface present mode count\n");
	}

	BLK_DEBUG("Got %u surface present mode(s)\n", surface_present_mode_count);

	if (surface_present_mode_count == 0)
	{
		BLK_FATAL("Surface present modes not found\n");
	}

	std::vector<VkPresentModeKHR> surface_present_modes(surface_present_mode_count);

	if (vkGetPhysicalDeviceSurfacePresentModesKHR(
			context.physical_device,
			context.surface,
			&surface_present_mode_count,
			surface_present_modes.data()
		) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to get surface present modes\n");
	}

	BLK_DEBUG("Finding the best settings for the swapchain...\n");
	uint32_t swapchain_min_image_count = surface_capabilities.minImageCount;

	if (surface_capabilities.maxImageCount == 0 ||
		surface_capabilities.maxImageCount > surface_capabilities.minImageCount)
	{
		swapchain_min_image_count += 1;
	}

	BLK_DEBUG("Swapchain min image count: %u\n", swapchain_min_image_count);

	if (surface_capabilities.currentExtent.width == UINT32_MAX &&
		surface_capabilities.currentExtent.height == UINT32_MAX)
	{
		BLK_FATAL("Surface size should be determined by the extend of the swapchain targeting the surface\n");
	}

	BLK_DEBUG("Selecting surface format...\n");
	bool found_best_surface_format = false;

	for (auto& surface_format : surface_formats)
	{
		if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
			surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			BLK_DEBUG("Found best surface format\n");
			swapchain.surface_format = surface_format;
			found_best_surface_format = true;
			break;
		}
	}

	if (!found_best_surface_format)
	{
		BLK_DEBUG("Could not find best surface format. Selecting default\n");
		swapchain.surface_format = surface_formats.front();
	}

	BLK_DEBUG("Selecting surface present mode...\n");
	const VkPresentModeKHR* selected_surface_present_mode = nullptr;

	for (const auto& present_mode : surface_present_modes)
	{
		if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			BLK_DEBUG("Found best surface present mode\n");
			selected_surface_present_mode = &present_mode;
			break;
		}
	}

	if (!selected_surface_present_mode)
	{
		BLK_DEBUG("Could not find best surface present mode. Selecting default\n");
		selected_surface_present_mode = &surface_present_modes.front();
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface = context.surface;
	swapchain_create_info.minImageCount = swapchain_min_image_count;
	swapchain_create_info.imageFormat = swapchain.surface_format.format;
	swapchain_create_info.imageColorSpace = swapchain.surface_format.colorSpace;
	swapchain_create_info.imageExtent = surface_capabilities.currentExtent;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.preTransform = surface_capabilities.currentTransform;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.presentMode = *selected_surface_present_mode;
	swapchain_create_info.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(context.logical_device, &swapchain_create_info, nullptr, &swapchain.swapchain) !=
		VK_SUCCESS)
	{
		BLK_FATAL("Failed to create swapchain\n");
	}

	swapchain.extent = surface_capabilities.currentExtent;
	swapchain.aspect_ratio = static_cast<float>(swapchain.extent.width) / static_cast<float>(swapchain.extent.height);

	BLK_DEBUG("Retrieving swap chain images...\n");
	uint32_t swapchain_image_count = 0;

	if (vkGetSwapchainImagesKHR(context.logical_device, swapchain.swapchain, &swapchain_image_count, nullptr))
	{
		BLK_FATAL("Failed to get swapchain image count\n");
	}

	BLK_DEBUG("Got %u swapchain image(s)\n", swapchain_image_count);

	if (swapchain_image_count == 0)
	{
		BLK_FATAL("Swapchain images not found\n");
	}

	swapchain.images = std::vector<VkImage>(swapchain_image_count);
	swapchain.image_views = std::vector<VkImageView>(swapchain_image_count);

	if (vkGetSwapchainImagesKHR(
			context.logical_device,
			swapchain.swapchain,
			&swapchain_image_count,
			swapchain.images.data()
		))
	{
		BLK_FATAL("Failed to get swapchain images\n");
	}

	BLK_DEBUG("Creating swapchain image views...\n");

	VkImageSubresourceRange image_subresource_range = {};
	image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_subresource_range.levelCount = 1;
	image_subresource_range.layerCount = 1;

	VkImageViewCreateInfo image_view_create_info = {};
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format = swapchain.surface_format.format;
	image_view_create_info.subresourceRange = image_subresource_range;

	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		image_view_create_info.image = swapchain.images[i];

		if (vkCreateImageView(context.logical_device, &image_view_create_info, nullptr, &swapchain.image_views[i]) !=
			VK_SUCCESS)
		{
			BLK_FATAL("Failed to create swapchain image view\n");
		}

		swapchain.render_finished_semaphores.push_back(create_semaphore(context));
	}

	return swapchain;
}
