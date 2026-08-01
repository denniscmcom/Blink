// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Resource/Material_Device.hpp"
#include "Engine/Renderer/Resource/Mesh_Device.hpp"

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace blk
{
struct Context;
struct Pipeline;

struct Camera_UBO
{
	Matrix4 view;
	Matrix4 projection;
};

constexpr uint32_t MAX_LIGHT_COUNT = 16;

// FIXME: This is fragile.
// Layout must match Shaders/Shared.slang (std140). Scalars are placed before
// the float3 arrays so the C++ (alignas(16) Vector3) and std140 layouts agree.
struct Light_UBO
{
	uint32_t light_count;
	float ambient_strength;
	Vector3 view_position;
	Vector3 light_positions[MAX_LIGHT_COUNT];
	Vector3 light_colors[MAX_LIGHT_COUNT];
};

struct Mesh_Constants
{
	Matrix4 model;
	Matrix4 normal_matrix;
};

struct Light_Constants
{
	uint32_t light_index;
};

struct Draw_Command
{
	Mesh_Constants mesh_constants;
	Mesh_Device mesh_device;
	Pipeline* pipeline;
	std::optional<Material_Device> material_device;
	Light_Constants light_constants;
};

struct Frame
{
	VkCommandBuffer command_buffer;
	VkSemaphore semaphore;
	VkFence fence;
	Buffer camera_buffer;
	Buffer light_buffer;
	VkDescriptorSet camera_descriptor_set;
	VkDescriptorSet light_descriptor_set;
	std::vector<Draw_Command> draw_commands;
};

Frame create_frame(
	const Context& context,
	VkDescriptorPool pool,
	VkDescriptorSetLayout camera_layout,
	VkDescriptorSetLayout light_layout
);
}  // namespace blk
