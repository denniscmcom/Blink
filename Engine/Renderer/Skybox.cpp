// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Skybox.hpp"

#include "Engine/Renderer/Helpers.hpp"

#include <vulkan/vulkan.h>

namespace
{
/// The color a skybox LUT is cleared to when `World_Settings` disables the pass that fills it.
constexpr VkClearColorValue BLACK_CLEAR_COLOR = {};

/// The whole of a skybox LUT. They all have a single mip level and a single array layer.
constexpr VkImageSubresourceRange LUT_SUBRESOURCE_RANGE = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseMipLevel = 0,
	.levelCount = 1,
	.baseArrayLayer = 0,
	.layerCount = 1,
};
}  // namespace

void
blk::clear_skybox_lut(VkCommandBuffer command_buffer, VkImage image, VkPipelineStageFlags2 destination_stage)
{
	// The old layout is `VK_IMAGE_LAYOUT_UNDEFINED` because we are about to overwrite every texel, so whatever the
	// image holds does not have to survive the transition.

	transition_image_layout(
		command_buffer,
		image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		{},
		VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_CLEAR_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	vkCmdClearColorImage(
		command_buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		&BLACK_CLEAR_COLOR,
		1,
		&LUT_SUBRESOURCE_RANGE
	);

	transition_image_layout(
		command_buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_ACCESS_2_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_2_CLEAR_BIT,
		destination_stage,
		VK_IMAGE_ASPECT_COLOR_BIT
	);
}
