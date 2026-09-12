// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Array.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"

#include <vulkan/vulkan.h>

namespace blk
{
enum class Result;

/// A swapchain.
struct Swapchain
{
	/// Vulkan swapchain.
	VkSwapchainKHR swapchain;
	/// Swapchain vulkan images.
	///
	/// There could be more images than `MAX_FRAMES_IN_FLIGHT`.
	Dyn_Array<VkImage> images;
	/// Swapchain vulkan image view.
	/// We cannot access Vulkan images directly.
	Dyn_Array<VkImageView> image_views;
	/// Vulkan surface format.
	VkSurfaceFormatKHR surface_format;
	/// Vulkan surface present mode.
	VkPresentModeKHR surface_present_mode;
	/// Swapchain image size.
	VkExtent2D extent;
	/// Swapchain aspect ratio.
	float aspect_ratio;
	/// To synchronize device-device for presentation.
	/// After submitting rendering commands, the queue signals the semaphore.
	/// `vkQueuePresentKHR` waits on it before displaying the image.
	///
	/// There is one semaphore per swapchain image, indexed by the image index returned by `vkAcquireNextImageKHR`.
	///
	/// @warning They cannot be indexed by frame in flight. `vkQueuePresentKHR` cannot signal a fence or a semaphore, so
	/// there is no way to know when it has finished waiting on one — a frame fence does not cover it. Acquiring an
	/// image index is what guarantees the previous presentation using that same index has completed, which is why these
	/// are the only frame resource indexed by image instead of by frame.
	Dyn_Array<VkSemaphore> render_finished_semaphores;
};

/// Creates a `swapchain` .
Result create_swapchain(const Context& context, Swapchain& swapchain);
/// Destroys a `swapchain`.
void destroy_swapchain(const Context& context, Swapchain& swapchain);
}  // namespace blk
