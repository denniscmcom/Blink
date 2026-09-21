// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Frame.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Lifetime/Command.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Lifetime/Descriptor.hpp"
#include "Engine/Renderer/Lifetime/Draw_Command.hpp"
#include "Engine/Renderer/Lifetime/Sync.hpp"

#include <vulkan/vulkan.h>

blk::Result
blk::create_frame(const Context& context, const Descriptor_Layouts& layouts, Frame& frame)
{
	frame = {};
	frame.descriptor_pool = layouts.pool;

	// Create camera buffer.

	Buffer camera_buffer = {};
	BLK_SUCCESS_OR_RETURN(create_buffer(
		context,
		sizeof(Camera_UBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		camera_buffer
	));

	if (const Result result = map_buffer(context, camera_buffer); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to map frame camera buffer\n");
		destroy_buffer(context, camera_buffer);

		return result;
	}

	frame.camera_buffer = camera_buffer;

	// Create light buffer.

	Buffer light_buffer = {};

	if (const Result result = create_buffer(
			context,
			sizeof(Light_UBO),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			light_buffer
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create frame light buffer\n");
		destroy_frame(context, frame);

		return result;
	}

	if (const Result result = map_buffer(context, light_buffer); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to map frame light buffer\n");
		destroy_buffer(context, light_buffer);
		destroy_frame(context, frame);

		return result;
	}

	frame.light_buffer = light_buffer;

	// Create skybox buffer.

	Buffer skybox_buffer = {};

	if (const Result result = create_buffer(
			context,
			sizeof(Skybox_UBO),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			skybox_buffer
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create frame skybox buffer\n");
		destroy_frame(context, frame);

		return result;
	}

	if (const Result result = map_buffer(context, skybox_buffer); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to map skybox buffer\n");
		destroy_buffer(context, skybox_buffer);
		destroy_frame(context, frame);

		return result;
	}

	frame.skybox_buffer = skybox_buffer;

	// Create synchronization primitives.

	if (const Result result = create_semaphore(context, frame.semaphore); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create frame semaphore\n");
		destroy_frame(context, frame);

		return result;
	}

	if (const Result result = create_fence(context, frame.fence); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create frame fence\n");
		destroy_frame(context, frame);

		return result;
	}

	// Create command buffer from the frame pool.

	if (const Result result = create_command_buffer(context, context.frame_command_pool, frame.command_buffer);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create frame command buffer\n");
		destroy_frame(context, frame);

		return result;
	}

	// Allocate descriptor set.

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = layouts.pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = &layouts.frame_layout;

	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;

	if (vkAllocateDescriptorSets(context.logical_device, &descriptor_set_allocate_info, &descriptor_set) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to allocate frame descriptor set\n");
		destroy_frame(context, frame);

		return Result::DEVICE_ERROR;
	}

	frame.descriptor_set = descriptor_set;

	// Currently, the descriptor set is allocated but empty.
	// We need to connect them to the actual buffers previously created.

	VkDescriptorBufferInfo camera_descriptor_buffer_info = {};
	camera_descriptor_buffer_info.buffer = frame.camera_buffer.buffer;
	camera_descriptor_buffer_info.range = frame.camera_buffer.size;

	VkDescriptorBufferInfo light_descriptor_buffer_info = {};
	light_descriptor_buffer_info.buffer = frame.light_buffer.buffer;
	light_descriptor_buffer_info.range = frame.light_buffer.size;

	VkDescriptorBufferInfo skybox_descriptor_buffer_info = {};
	skybox_descriptor_buffer_info.buffer = frame.skybox_buffer.buffer;
	skybox_descriptor_buffer_info.range = frame.skybox_buffer.size;

	const Array<VkWriteDescriptorSet, 3> descriptor_writes = {{
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = frame.descriptor_set,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &camera_descriptor_buffer_info,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = frame.descriptor_set,
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &light_descriptor_buffer_info,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = frame.descriptor_set,
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &skybox_descriptor_buffer_info,
		},
	}};

	vkUpdateDescriptorSets(context.logical_device, descriptor_writes.capacity, descriptor_writes.buffer, 0, nullptr);

	// Create the draw command arrays.

	if (const Result result = create_dyn_array(frame.mesh_draw_commands, context.allocator, INITIAL_DRAW_COMMAND_COUNT);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create frame mesh draw commands array\n");
		destroy_frame(context, frame);

		return result;
	}

	if (const Result result =
			create_dyn_array(frame.skybox_draw_commands, context.allocator, INITIAL_DRAW_COMMAND_COUNT);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create frame skybox draw commands array\n");
		destroy_frame(context, frame);

		return result;
	}

	return Result::SUCCESS;
}

void
blk::destroy_frame(const Context& context, Frame& frame)
{
	// Destroy draw arrays.

	destroy_dyn_array(frame.mesh_draw_commands);
	destroy_dyn_array(frame.skybox_draw_commands);

	// Destroy descriptor set.

	// `frame.descriptor_pool` was created with `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`, so we can return
	// the sets to it. A `VK_NULL_HANDLE` element is ignored, so we do not have to check them.
	vkFreeDescriptorSets(context.logical_device, frame.descriptor_pool, 1, &frame.descriptor_set);

	// Destroy buffers.

	destroy_buffer(context, frame.light_buffer);
	destroy_buffer(context, frame.camera_buffer);
	destroy_buffer(context, frame.skybox_buffer);

	destroy_command_buffer(context, context.frame_command_pool, frame.command_buffer);

	// Destroy synchronization primitives.

	destroy_fence(context, frame.fence);
	destroy_semaphore(context, frame.semaphore);

	frame = {};
}
