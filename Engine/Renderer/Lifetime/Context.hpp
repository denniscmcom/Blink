// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>
#include <windows.h>

#include <stdint.h>

namespace blk
{
struct Allocator;

/// Maximum number of frames being processed concurrently.
///
/// It should be two at a minimum, so that the CPU and GPU can work on their own tasks at the same time.
/// By having too many frames in flight, the CPU could get too far ahead of the GPU, adding latency.
///
/// Any resource that is accessed and modified during rendering must be duplicated `MAX_FRAMES_IN_FLIGHT` times.
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

/// The renderer's context is created once when starting the renderer, and live until the renderer is destroyed. It is
/// usually read-only data and it can be accessed by any other part of the renderer.
struct Context
{
	/// Vulkan API version.
	uint32_t api_version;
	/// Instance.
	VkInstance instance = VK_NULL_HANDLE;
	/// Debug messenger.
	/// It routes validation layer output to `Platform/Log.hpp`.
	VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
	/// Surface.
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	/// Selected physical device.
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	/// Logical device.
	VkDevice logical_device = VK_NULL_HANDLE;
	/// Queue to render graphics and compute.
	VkQueue queue = VK_NULL_HANDLE;
	/// Selected queue family.
	/// `UINT32_MAX` is used as a sentinel value to represent no queue family index yet.
	uint32_t queue_family_index = UINT32_MAX;

	/// Frame command pool.
	/// Used to records all commands in a frame.
	/// Created using `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`.
	VkCommandPool frame_command_pool = VK_NULL_HANDLE;
	/// Transient command pool.
	/// Used to record short-lived commands in any point.
	/// Created using `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`.
	VkCommandPool transient_command_pool = VK_NULL_HANDLE;

	/// Sampler for textures.
	VkSampler texture_sampler = VK_NULL_HANDLE;
	/// Sampler for lookup tables.
	VkSampler lut_sampler = VK_NULL_HANDLE;

	/// Renderer host allocator.
	Allocator* allocator;
};

/// Creates a `Context`.
/// @param allocator Renderer host allocator.
/// @param window The Win32 window handle.
Context create_context(Allocator* allocator, HWND window);
/// Destroys a `context`.
void destroy_context(Context& context);
}  // namespace blk
