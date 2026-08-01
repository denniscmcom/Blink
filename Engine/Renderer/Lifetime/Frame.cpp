// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Frame.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Renderer/Helpers.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"

#include <vulkan/vulkan.h>

#include <array>

blk::Frame
blk::create_frame(
	const Context& context,
	VkDescriptorPool pool,
	VkDescriptorSetLayout camera_layout,
	VkDescriptorSetLayout light_layout
)
{
	Buffer camera_buffer = create_buffer(
		context,
		sizeof(Camera_UBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	map_buffer(context, camera_buffer);

	Buffer light_buffer = create_buffer(
		context,
		sizeof(Light_UBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	map_buffer(context, light_buffer);

	VkSemaphore semaphore = create_semaphore(context);
	VkFence fence = create_fence(context);
	VkCommandBuffer command_buffer = create_command_buffer(context, context.frame_command_pool);

	const std::array layouts = {camera_layout, light_layout};

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = pool;
	descriptor_set_allocate_info.descriptorSetCount = layouts.size();
	descriptor_set_allocate_info.pSetLayouts = layouts.data();

	std::vector<VkDescriptorSet> descriptor_sets(descriptor_set_allocate_info.descriptorSetCount);

	if (vkAllocateDescriptorSets(context.logical_device, &descriptor_set_allocate_info, descriptor_sets.data()) !=
		VK_SUCCESS)
	{
		BLK_FATAL("Failed to allocate descriptor sets\n");
	}

	VkDescriptorBufferInfo camera_descriptor_buffer_info = {};
	camera_descriptor_buffer_info.buffer = camera_buffer.buffer;
	camera_descriptor_buffer_info.range = camera_buffer.size;

	VkDescriptorBufferInfo light_descriptor_buffer_info = {};
	light_descriptor_buffer_info.buffer = light_buffer.buffer;
	light_descriptor_buffer_info.range = light_buffer.size;

	const std::array<VkWriteDescriptorSet, 2> descriptor_writes = {{
		{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		 .dstSet = descriptor_sets[0],
		 .dstBinding = 0,
		 .dstArrayElement = 0,
		 .descriptorCount = 1,
		 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		 .pBufferInfo = &camera_descriptor_buffer_info},
		{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		 .dstSet = descriptor_sets[1],
		 .dstBinding = 0,
		 .dstArrayElement = 0,
		 .descriptorCount = 1,
		 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		 .pBufferInfo = &light_descriptor_buffer_info},
	}};

	vkUpdateDescriptorSets(context.logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

	return Frame{
		.command_buffer = command_buffer,
		.semaphore = semaphore,
		.fence = fence,
		.camera_buffer = camera_buffer,
		.light_buffer = light_buffer,
		.camera_descriptor_set = descriptor_sets[0],
		.light_descriptor_set = descriptor_sets[1],
	};
}
