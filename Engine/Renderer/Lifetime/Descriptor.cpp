// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Descriptor.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Resource/Arena.hpp"

#include <vulkan/vulkan.h>

#include <array>

blk::Descriptor_Layouts
blk::create_descriptor_layouts(const Context& context)
{
	constexpr std::array<VkDescriptorPoolSize, 2> descriptor_pool_sizes = {{
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = MAX_FRAMES_IN_FLIGHT * 2 + MAX_MATERIAL_COUNT,
		},
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = MAX_MATERIAL_COUNT * 3,
		},
	}};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
	descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptor_pool_create_info.maxSets = MAX_FRAMES_IN_FLIGHT * 2 + MAX_MATERIAL_COUNT;
	descriptor_pool_create_info.poolSizeCount = descriptor_pool_sizes.size();
	descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();

	VkDescriptorPool pool;

	if (vkCreateDescriptorPool(context.logical_device, &descriptor_pool_create_info, nullptr, &pool) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create descriptor pool\n");
	}

	constexpr std::array<VkDescriptorSetLayoutBinding, 1> camera_bindings = {{
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	}};

	VkDescriptorSetLayoutCreateInfo camera_layout_create_info = {};
	camera_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	camera_layout_create_info.bindingCount = camera_bindings.size();
	camera_layout_create_info.pBindings = camera_bindings.data();

	VkDescriptorSetLayout camera_layout;

	if (vkCreateDescriptorSetLayout(context.logical_device, &camera_layout_create_info, nullptr, &camera_layout) !=
		VK_SUCCESS)
	{
		BLK_FATAL("Failed to create descriptor set layout\n");
	}

	constexpr std::array<VkDescriptorSetLayoutBinding, 1> light_bindings = {{
		{.binding = 0,
		 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		 .descriptorCount = 1,
		 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
	}};

	VkDescriptorSetLayoutCreateInfo light_layout_info = {};
	light_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	light_layout_info.bindingCount = 1;
	light_layout_info.pBindings = light_bindings.data();

	VkDescriptorSetLayout light_layout;

	if (vkCreateDescriptorSetLayout(context.logical_device, &light_layout_info, nullptr, &light_layout) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create descriptor set layout\n");
	}

	constexpr std::array<VkDescriptorSetLayoutBinding, 4> material_bindings = {{
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	}};

	VkDescriptorSetLayoutCreateInfo material_layout_create_info = {};
	material_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	material_layout_create_info.bindingCount = material_bindings.size();
	material_layout_create_info.pBindings = material_bindings.data();

	VkDescriptorSetLayout material_layout;

	if (vkCreateDescriptorSetLayout(context.logical_device, &material_layout_create_info, nullptr, &material_layout) !=
		VK_SUCCESS)
	{
		BLK_FATAL("Failed to create descriptor set layout\n");
	}

	return Descriptor_Layouts{
		.pool = pool,
		.camera_layout = camera_layout,
		.light_layout = light_layout,
		.material_layout = material_layout,
	};
}
