// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>

#include <stdint.h>

namespace blk
{
/// Width of the skybox transmittance LUT.
constexpr uint32_t SKYBOX_TRANSMITTANCE_LUT_WIDTH = 256;
/// Height of the skybox transmittance LUT.
constexpr uint32_t SKYBOX_TRANSMITTANCE_LUT_HEIGHT = 64;

/// Width of the skybox multiscattering LUT.
constexpr uint32_t SKYBOX_MULTISCATTERING_LUT_WIDTH = 32;
/// Height of the skybox multiscattering LUT.
constexpr uint32_t SKYBOX_MULTISCATTERING_LUT_HEIGHT = 32;

/// Width of the skybox sky-view LUT.
constexpr uint32_t SKYBOX_SKY_VIEW_LUT_WIDTH = 200;
/// Height of the skybox sky-view LUT.
constexpr uint32_t SKYBOX_SKY_VIEW_LUT_HEIGHT = 100;

/// Width of the skybox aerial LUT.
constexpr uint32_t SKYBOX_AERIAL_LUT_WIDTH = 32;
/// Height of the skybox aerial LUT.
constexpr uint32_t SKYBOX_AERIAL_LUT_HEIGHT = 32;
/// Depth of the skybox aerial LUT.
constexpr uint32_t SKYBOX_AERIAL_LUT_DEPTH = 32;

/// Workgroup size of skybox transmittance shader.
/// It has to match the `numthreads` declared in `Shaders/CS_Skybox_Transmittance.slang`.
constexpr uint32_t SKYBOX_TRANSMITTANCE_WORKGROUP_SIZE = 8;

/// Workgroup size of skybox multiscattering shader.
/// It has to match the `numthreads` declared in `Shaders/CS_Skybox_Multiscattering.slang`.
constexpr uint32_t SKYBOX_MULTISCATTERING_WORKGROUP_SIZE = 8;

/// Workgroup size of skybox sky-view shader.
/// It has to match the `numthreads` declared in `Shaders/CS_Skybox_Sky_View.slang`.
constexpr uint32_t SKYBOX_SKY_VIEW_WORKGROUP_SIZE = 5;

/// Workgroup size of skybox aerial shader.
/// It has to match the `numthreads` declared in `Shaders/CS_Skybox_Aerial.slang`.
constexpr uint32_t SKYBOX_AERIAL_WORKGROUP_SIZE = 8;

// The dispatch below rounds down, so a LUT that is not a whole number of workgroups would leave texels unwritten.

static_assert(SKYBOX_TRANSMITTANCE_LUT_WIDTH % SKYBOX_TRANSMITTANCE_WORKGROUP_SIZE == 0);
static_assert(SKYBOX_TRANSMITTANCE_LUT_HEIGHT % SKYBOX_TRANSMITTANCE_WORKGROUP_SIZE == 0);

static_assert(SKYBOX_MULTISCATTERING_LUT_WIDTH % SKYBOX_MULTISCATTERING_WORKGROUP_SIZE == 0);
static_assert(SKYBOX_MULTISCATTERING_LUT_HEIGHT % SKYBOX_MULTISCATTERING_WORKGROUP_SIZE == 0);

static_assert(SKYBOX_SKY_VIEW_LUT_WIDTH % SKYBOX_SKY_VIEW_WORKGROUP_SIZE == 0);
static_assert(SKYBOX_SKY_VIEW_LUT_HEIGHT % SKYBOX_SKY_VIEW_WORKGROUP_SIZE == 0);

// `SKYBOX_AERIAL_LUT_DEPTH` is not checked: the aerial LUT is dispatched over its width and height only, and each
// invocation walks every depth slice of its own froxel column.

static_assert(SKYBOX_AERIAL_LUT_WIDTH % SKYBOX_AERIAL_WORKGROUP_SIZE == 0);
static_assert(SKYBOX_AERIAL_LUT_HEIGHT % SKYBOX_AERIAL_WORKGROUP_SIZE == 0);

/// Clears a skybox LUT to black and leaves it in `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`.
///
/// A LUT outlives the frame that filled it, so a pass that `World_Settings` disables cannot just skip its dispatch: the
/// image would keep the texels the last enabled frame wrote, which are the same ones the skipped pass would have
/// recomputed, and nothing on screen would change. Clearing it is what makes the pass's contribution visible, and it
/// also gives whoever samples the LUT a defined layout on a pass that has not run since startup.
///
/// @param destination_stage The pipeline stages that sample the LUT after the clear.
void clear_skybox_lut(VkCommandBuffer command_buffer, VkImage image, VkPipelineStageFlags2 destination_stage);
}  // namespace blk
