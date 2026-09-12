// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Descriptor.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Resource/Arena.hpp"

#include <vulkan/vulkan.h>

blk::Result
blk::create_descriptor_layouts(const Context& context, Descriptor_Layouts& layouts)
{
	layouts = {};

	// Create descriptor set pool.

	// Pre-sized pool for everything the renderer will ever need.
	// How many descriptor sets we need for each type.
	constexpr Array<VkDescriptorPoolSize, 2> descriptor_pool_sizes = {{
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			// We need 1 per frame in flight * 2 (camera UBO + light UBO) + 1 UBO per material.
			.descriptorCount = MAX_FRAMES_IN_FLIGHT * 2 + MAX_MATERIAL_COUNT,
		},
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			// Three textures per material (albedo, normal, orm).
			.descriptorCount = MAX_MATERIAL_COUNT * 3,
		},
	}};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
	descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	// How many descriptor sets total we can allocate from this pool.
	// We have 2 sets per frame in flight (camera + light), plus one set per material.
	descriptor_pool_create_info.maxSets = MAX_FRAMES_IN_FLIGHT * 2 + MAX_MATERIAL_COUNT;
	descriptor_pool_create_info.poolSizeCount = descriptor_pool_sizes.capacity;
	descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.buffer;

	VkDescriptorPool pool = VK_NULL_HANDLE;

	if (vkCreateDescriptorPool(context.logical_device, &descriptor_pool_create_info, nullptr, &pool) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create descriptor pool\n");

		return Result::DEVICE_ERROR;
	}

	layouts.pool = pool;

	// Create camera descriptor set layout.

	constexpr Array<VkDescriptorSetLayoutBinding, 1> camera_bindings = {{
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	}};

	VkDescriptorSetLayoutCreateInfo camera_layout_create_info = {};
	camera_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	camera_layout_create_info.bindingCount = camera_bindings.capacity;
	camera_layout_create_info.pBindings = camera_bindings.buffer;

	VkDescriptorSetLayout camera_layout = VK_NULL_HANDLE;

	if (vkCreateDescriptorSetLayout(context.logical_device, &camera_layout_create_info, nullptr, &camera_layout) !=
		VK_SUCCESS)
	{
		BLK_ERROR("Failed to create camera descriptor set layout\n");
		destroy_descriptor_layouts(context, layouts);

		return Result::DEVICE_ERROR;
	}

	layouts.camera_layout = camera_layout;

	// Create light descriptor set layout.

	constexpr Array<VkDescriptorSetLayoutBinding, 1> light_bindings = {{
		{.binding = 0,
		 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		 .descriptorCount = 1,
		 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
	}};

	VkDescriptorSetLayoutCreateInfo light_layout_info = {};
	light_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	light_layout_info.bindingCount = light_bindings.capacity;
	light_layout_info.pBindings = light_bindings.buffer;

	VkDescriptorSetLayout light_layout = VK_NULL_HANDLE;

	if (vkCreateDescriptorSetLayout(context.logical_device, &light_layout_info, nullptr, &light_layout) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create light descriptor set layout\n");
		destroy_descriptor_layouts(context, layouts);

		return Result::DEVICE_ERROR;
	}

	layouts.light_layout = light_layout;

	// Create material descriptor set layout.

	constexpr Array<VkDescriptorSetLayoutBinding, 4> material_bindings = {{
		// Albedo texture.
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		// Normal texture.
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		// ORM texture.
		{
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		// `Material_UBO`. It is written by `transfer_material`.
		{
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	}};

	VkDescriptorSetLayoutCreateInfo material_layout_create_info = {};
	material_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	material_layout_create_info.bindingCount = material_bindings.capacity;
	material_layout_create_info.pBindings = material_bindings.buffer;

	VkDescriptorSetLayout material_layout = VK_NULL_HANDLE;

	if (vkCreateDescriptorSetLayout(context.logical_device, &material_layout_create_info, nullptr, &material_layout) !=
		VK_SUCCESS)
	{
		BLK_ERROR("Failed to create descriptor set layout\n");
		destroy_descriptor_layouts(context, layouts);

		return Result::DEVICE_ERROR;
	}

	layouts.material_layout = material_layout;

	return Result::SUCCESS;
}

void
blk::destroy_descriptor_layouts(const Context& context, Descriptor_Layouts& layouts)
{
	vkDestroyDescriptorSetLayout(context.logical_device, layouts.material_layout, nullptr);
	vkDestroyDescriptorSetLayout(context.logical_device, layouts.light_layout, nullptr);
	vkDestroyDescriptorSetLayout(context.logical_device, layouts.camera_layout, nullptr);

	// Destroying the pool frees every descriptor set allocated from it.
	vkDestroyDescriptorPool(context.logical_device, layouts.pool, nullptr);

	layouts = {};
}
