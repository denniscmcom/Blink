// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Resource/Material_Device.hpp"
#include "Engine/Renderer/Resource/Mesh_Device.hpp"

#include <vulkan/vulkan.h>

namespace blk
{
struct Context;
struct Pipeline;
enum class Result;

/// Camera uniform buffer object.
///
/// It lives the entire frame.
struct Camera_UBO
{
	/// View matrix.
	Matrix4 view;
	/// Projection matrix.
	Matrix4 projection;
	/// View/eye position.
	Vector3 view_position;
};

/// Maximum amount of lights in a scene supported by the renderer.
constexpr uint32_t MAX_LIGHT_COUNT = 16;

/// Light uniform buffer object.
///
/// It lives the entire frame.
struct Light_UBO
{
	/// Amount of lights in the scene.
	uint32_t light_count;
	/// Array of light positions.
	Vector3 light_positions[MAX_LIGHT_COUNT];
	/// Array of light colors.
	Vector3 light_colors[MAX_LIGHT_COUNT];
};

/// Per-mesh parameters.
struct Mesh_Constants
{
	/// Mesh model matrix.
	Matrix4 model;
	/// Mesh normal matrix.
	Matrix4 normal_matrix;
};

/// Per-light parameters.
struct Light_Constants
{
	/// Index of this light in `Light_UBO`'s position and color arrays, so that the fragment shader can look it up.
	uint32_t light_index;
};

/// Data needed by the renderer to issue a single draw command.
///
/// Many draw commands are issued per frame.
struct Draw_Command
{
	/// Whether this is a mesh or a light is decided by which array it lives in — `Frame::mesh_draw_commands` or
	/// `Frame::light_draw_commands` — so only the fields relevant to that array are populated.

	/// Pipeline to use when issuing the draw command.
	Pipeline* pipeline;
	/// Mesh parameters.
	Mesh_Constants mesh_constants;
	/// Light parameters.
	Light_Constants light_constants;
	///	Mesh data uploaded to device.
	Mesh_Device mesh_device;
	/// Material data uploaded to device.
	Material_Device material_device;
};

/// Initial number of draw commands a frame can hold before its arrays have to grow.
constexpr size_t INITIAL_DRAW_COMMAND_COUNT = 64;

/// Data needed by each frame.
struct Frame
{
	/// Frame command buffer to record commands to.
	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	/// To synchronize device-device. The host never sees it.
	/// Coordinates work between queue submissions without involving the host.
	VkSemaphore semaphore = VK_NULL_HANDLE;
	/// To Synchronize device-host. The host waits.
	/// In `update_frame` the host waits until the device has finished executing this frame's previous submission.
	/// Without it, we'd start overwriting `command_buffer` and UBO data while the device is still reading them.
	VkFence fence = VK_NULL_HANDLE;
	/// Camera buffer to store `Camera_UBO`.
	Buffer camera_buffer;
	/// Light buffer to store `Light_UBO`.
	Buffer light_buffer;
	/// The `Descriptor_Layouts::pool` the descriptor sets below were allocated from. We keep it so that `destroy_frame`
	/// can return them to it.
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	/// Camera descriptor set allocated from `descriptor_pool`.
	VkDescriptorSet camera_descriptor_set = VK_NULL_HANDLE;
	/// Light descriptor set allocated from `descriptor_pool`.
	VkDescriptorSet light_descriptor_set = VK_NULL_HANDLE;
	/// Array of draw commands to render meshes.
	Dyn_Array<Draw_Command> mesh_draw_commands;
	/// Array of draw commands to render lights.
	Dyn_Array<Draw_Command> light_draw_commands;
};

/// Creates `frame` data.
Result create_frame(const Context& context, const Descriptor_Layouts& layouts, Frame& frame);
/// Destroys a `frame`.
void destroy_frame(const Context& context, Frame& frame);
}  // namespace blk
