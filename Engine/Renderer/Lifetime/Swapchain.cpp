// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Swapchain.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Helpers.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Lifetime/Sync.hpp"

#include <vulkan/vulkan.h>

blk::Result
blk::create_swapchain(const Context& context, Swapchain& swapchain)
{
	swapchain = {};

	// Get surface format.

	// Get surface capabilities.
	VkSurfaceCapabilitiesKHR surface_capabilities = {};

	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physical_device, context.surface, &surface_capabilities) !=
		VK_SUCCESS)
	{
		BLK_ERROR("Failed to get surface capabilities\n");

		return Result::DEVICE_ERROR;
	}

	swapchain.extent = surface_capabilities.currentExtent;
	swapchain.aspect_ratio = static_cast<float>(swapchain.extent.width) / static_cast<float>(swapchain.extent.height);

	// Get how many surface formats are supported.
	uint32_t surface_format_count = 0;

	if (vkGetPhysicalDeviceSurfaceFormatsKHR(
			context.physical_device,
			context.surface,
			&surface_format_count,
			nullptr
		) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to get surface format count\n");

		return Result::DEVICE_ERROR;
	}

	if (surface_format_count == 0)
	{
		BLK_ERROR("Surface formats not found\n");

		return Result::DEVICE_ERROR;
	}

	// Get all supported surface formats.
	Dyn_Array<VkSurfaceFormatKHR> surface_formats = {};

	if (const Result result = create_dyn_array(surface_formats, context.allocator, surface_format_count);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create surface formats dynamic array\n");

		return result;
	}

	if (vkGetPhysicalDeviceSurfaceFormatsKHR(
			context.physical_device,
			context.surface,
			&surface_format_count,
			surface_formats.buffer
		) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to get surface formats\n");
		destroy_swapchain(context, swapchain);

		return Result::DEVICE_ERROR;
	}

	// Vulkan writes into `buffer` without touching `count`, so we set it ourselves.
	surface_formats.count = surface_format_count;

	// Now we get the supported surface present modes to compare them later with the supported surface formats and
	// select the best match.
	uint32_t surface_present_mode_count = 0;

	if (vkGetPhysicalDeviceSurfacePresentModesKHR(
			context.physical_device,
			context.surface,
			&surface_present_mode_count,
			nullptr
		) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to get surface present mode count\n");
		destroy_swapchain(context, swapchain);

		return Result::DEVICE_ERROR;
	}

	if (surface_present_mode_count == 0)
	{
		BLK_ERROR("Surface present modes not found\n");
		destroy_swapchain(context, swapchain);

		return Result::DEVICE_ERROR;
	}

	// Get supported surface present modes.
	Dyn_Array<VkPresentModeKHR> surface_present_modes = {};

	if (const Result result = create_dyn_array(surface_present_modes, context.allocator, surface_present_mode_count);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create surface present modes dynamic array\n");
		destroy_swapchain(context, swapchain);

		return result;
	}

	if (vkGetPhysicalDeviceSurfacePresentModesKHR(
			context.physical_device,
			context.surface,
			&surface_present_mode_count,
			surface_present_modes.buffer
		) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to get surface present modes\n");
		destroy_swapchain(context, swapchain);
		destroy_dyn_array(surface_present_modes);
		destroy_dyn_array(surface_formats);

		return Result::DEVICE_ERROR;
	}

	surface_present_modes.count = surface_present_mode_count;

	// Find the minimum image count.
	uint32_t swapchain_min_image_count = surface_capabilities.minImageCount;

	// `maxImageCount == 0` means no upper limit.
	if (surface_capabilities.maxImageCount == 0 ||
		surface_capabilities.maxImageCount > surface_capabilities.minImageCount)
	{
		// We request one more image to avoid stalling. With exactly `minImageCount`, the host might have to wait for
		// the presentation engine to release an image before it can acquire the next one.
		swapchain_min_image_count += 1;
	}

	// `UINT32_MAX` is a sentinel value that mean the window manager does not dictate the surface size.
	if (surface_capabilities.currentExtent.width == UINT32_MAX &&
		surface_capabilities.currentExtent.height == UINT32_MAX)
	{
		// TODO (Bug): Right now we are treating it as an error. However we should handle it properly.
		// We should query the size of our window and clamp to the allowed range.
		BLK_ERROR("Surface size should be determined by the extent of the swapchain targeting the surface\n");
		destroy_swapchain(context, swapchain);
		destroy_dyn_array(surface_present_modes);
		destroy_dyn_array(surface_formats);

		return Result::INVALID_ARGUMENTS;
	}

	// Now we select the best surface format.
	bool found_best_surface_format = false;

	for (size_t i = 0; i < surface_formats.count; ++i)
	{
		// This combination is preferred because is the most widely supported, and sRGB nonlinear means the device
		// applied gamma correction automatically – our fragment shaders output linear colors and the display gets
		// correct gamma without doing manual conversions.
		if (const VkSurfaceFormatKHR surface_format = surface_formats.buffer[i];
			surface_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
			surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			swapchain.surface_format = surface_format;
			found_best_surface_format = true;
			break;
		}
	}

	if (!found_best_surface_format)
	{
		// If we failed to find our preferred combination, we fallback to the first one in the array.
		BLK_WARNING("Failed to find best surface format: using fallback\n");
		swapchain.surface_format = surface_formats.buffer[0];
	}

	// Select best surface present mode.
	bool found_surface_best_present_mode = false;

	for (size_t i = 0; i < surface_present_modes.count; ++i)
	{
		// We select triple-buffered vsync.
		if (const VkPresentModeKHR surface_present_mode = surface_present_modes.buffer[i];
			surface_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			swapchain.surface_present_mode = surface_present_mode;
			found_surface_best_present_mode = true;
			break;
		}
	}

	if (!found_surface_best_present_mode)
	{
		// `VK_PRESENT_MODE_FIFO_KHR` is the only mode the specification guarantees is always supported, so it is a
		// safer fallback than whatever happens to be first in the array.
		BLK_WARNING("Failed to find best surface present mode: using fallback\n");
		swapchain.surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
	}

	// We can destroy these arrays because we are done selecting from them.
	destroy_dyn_array(surface_present_modes);
	destroy_dyn_array(surface_formats);

	// Create swapchain.

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
	swapchain_create_info.presentMode = swapchain.surface_present_mode;
	swapchain_create_info.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(context.logical_device, &swapchain_create_info, nullptr, &swapchain.swapchain) !=
		VK_SUCCESS)
	{
		BLK_ERROR("Failed to create swapchain\n");
		destroy_swapchain(context, swapchain);

		return Result::DEVICE_ERROR;
	}

	// Retrieve swapchain images.
	uint32_t swapchain_image_count = 0;

	if (vkGetSwapchainImagesKHR(context.logical_device, swapchain.swapchain, &swapchain_image_count, nullptr) !=
		VK_SUCCESS)
	{
		BLK_ERROR("Failed to get swapchain image count\n");
		destroy_swapchain(context, swapchain);

		return Result::DEVICE_ERROR;
	}

	if (swapchain_image_count == 0)
	{
		BLK_ERROR("Swapchain images not found\n");
		destroy_swapchain(context, swapchain);

		return Result::DEVICE_ERROR;
	}

	if (const Result result = create_dyn_array(swapchain.images, context.allocator, swapchain_image_count);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to allocate swapchain images dynamic array\n");
		destroy_swapchain(context, swapchain);

		return result;
	}

	if (const Result result = create_dyn_array(swapchain.image_views, context.allocator, swapchain_image_count);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to allocate swapchain image views dynamic array\n");
		destroy_swapchain(context, swapchain);

		return result;
	}

	if (vkGetSwapchainImagesKHR(
			context.logical_device,
			swapchain.swapchain,
			&swapchain_image_count,
			swapchain.images.buffer
		) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to get swapchain images\n");
		destroy_swapchain(context, swapchain);

		return Result::DEVICE_ERROR;
	}

	swapchain.images.count = swapchain_image_count;

	// Create image views.

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
		image_view_create_info.image = swapchain.images.buffer[i];

		if (vkCreateImageView(
				context.logical_device,
				&image_view_create_info,
				nullptr,
				&swapchain.image_views.buffer[i]
			) != VK_SUCCESS)
		{
			BLK_ERROR("Failed to create swapchain image view #%u\n", i);
			destroy_swapchain(context, swapchain);

			return Result::DEVICE_ERROR;
		}

		// We fill `buffer` directly instead of using `push`, so we keep `count` in step ourselves. `destroy_swapchain`
		// walks it to destroy the views created so far, including on the failure path above.
		swapchain.image_views.count += 1;
	}

	// Create render finished semaphores. There is one per swapchain image, not one per frame in flight.

	if (const Result result =
			create_dyn_array(swapchain.render_finished_semaphores, context.allocator, swapchain_image_count);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to allocate render finished semaphores dynamic array\n");
		destroy_swapchain(context, swapchain);

		return result;
	}

	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		if (const Result result = create_semaphore(context, swapchain.render_finished_semaphores.buffer[i]);
			result != Result::SUCCESS)
		{
			BLK_ERROR("Failed to create render finished semaphore #%u\n", i);
			destroy_swapchain(context, swapchain);

			return Result::DEVICE_ERROR;
		}

		swapchain.render_finished_semaphores.count += 1;
	}

	return Result::SUCCESS;
}

void
blk::destroy_swapchain(const Context& context, Swapchain& swapchain)
{
	for (size_t i = 0; i < swapchain.render_finished_semaphores.count; ++i)
	{
		destroy_semaphore(context, swapchain.render_finished_semaphores.buffer[i]);
	}

	destroy_dyn_array(swapchain.render_finished_semaphores);

	// The swapchain owns its images, so we only destroy the views we created for them.
	for (size_t i = 0; i < swapchain.image_views.count; ++i)
	{
		vkDestroyImageView(context.logical_device, swapchain.image_views.buffer[i], nullptr);
	}

	destroy_dyn_array(swapchain.image_views);
	destroy_dyn_array(swapchain.images);

	vkDestroySwapchainKHR(context.logical_device, swapchain.swapchain, nullptr);

	swapchain = {};
}
