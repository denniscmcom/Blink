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
	constexpr Array descriptor_pool_sizes = {{
		VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			// 3 UBOs (camera, light, skybox) * frame + 1 material UBO * material count.
			.descriptorCount = 3 * MAX_FRAMES_IN_FLIGHT + MAX_MATERIAL_COUNT,
		},
		VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			// 3 textures (albedo, normal, ORM) * material count + (skybox transmittance LUT + skybox multiscattering
			// LUT + skybox sky-view LUT + skybox aerial LUT) * frame.
			.descriptorCount = 3 * MAX_MATERIAL_COUNT + 4 * MAX_FRAMES_IN_FLIGHT,
		},
		VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			// (skybox transmittance LUT + skybox multiscattering LUT + skybox sky-view LUT + skybox aerial LUT) *
			// frame.
			.descriptorCount = 4 * MAX_FRAMES_IN_FLIGHT,
		},
	}};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
	descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	// How many descriptor sets total we can allocate from this pool. This counts sets, not the descriptors within them:
	// 1 frame set * frame + 1 material set * material count + 1 global set.
	descriptor_pool_create_info.maxSets = MAX_FRAMES_IN_FLIGHT + MAX_MATERIAL_COUNT + 1;
	descriptor_pool_create_info.poolSizeCount = descriptor_pool_sizes.capacity;
	descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.buffer;

	VkDescriptorPool pool = VK_NULL_HANDLE;

	if (vkCreateDescriptorPool(context.logical_device, &descriptor_pool_create_info, nullptr, &pool) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create descriptor pool\n");

		return Result::DEVICE_ERROR;
	}

	layouts.pool = pool;

	// Create descriptor set layout per frame in flight.

	constexpr Array frame_bindings = {{
		// Camera UBO.
		VkDescriptorSetLayoutBinding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		// Light UBO.
		VkDescriptorSetLayoutBinding{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		// Skybox UBO.
		VkDescriptorSetLayoutBinding{
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
		},
		// Write skybox transmittance LUT.
		VkDescriptorSetLayoutBinding{
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		},
		// Sample skybox transmittance LUT.
		VkDescriptorSetLayoutBinding{
			.binding = 4,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
		},
		// Write skybox multiscattering LUT.
		VkDescriptorSetLayoutBinding{
			.binding = 5,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		},
		// Sample skybox multiscattering LUT.
		VkDescriptorSetLayoutBinding{
			.binding = 6,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
		},
		// Write skybox sky-view LUT.
		VkDescriptorSetLayoutBinding{
			.binding = 7,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		},
		// Sample skybox sky-view LUT.
		VkDescriptorSetLayoutBinding{
			.binding = 8,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		// Write skybox aerial LUT.
		VkDescriptorSetLayoutBinding{
			.binding = 9,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		},
		// Sample skybox aerial LUT.
		VkDescriptorSetLayoutBinding{
			.binding = 10,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	}};

	VkDescriptorSetLayoutCreateInfo frame_layout_create_info = {};
	frame_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	frame_layout_create_info.bindingCount = frame_bindings.capacity;
	frame_layout_create_info.pBindings = frame_bindings.buffer;

	VkDescriptorSetLayout frame_layout = VK_NULL_HANDLE;

	if (vkCreateDescriptorSetLayout(context.logical_device, &frame_layout_create_info, nullptr, &frame_layout) !=
		VK_SUCCESS)
	{
		BLK_ERROR("Failed to create per frame in flight descriptor set layout\n");
		destroy_descriptor_layouts(context, layouts);

		return Result::DEVICE_ERROR;
	}

	layouts.frame_layout = frame_layout;

	// Create descriptor set layout per material.

	constexpr Array material_bindings = {{
		// Material UBO.
		VkDescriptorSetLayoutBinding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		// Albedo texture.
		VkDescriptorSetLayoutBinding{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		// Normal texture.
		VkDescriptorSetLayoutBinding{
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		// ORM texture.
		VkDescriptorSetLayoutBinding{
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
		BLK_ERROR("Failed to create per material descriptor set layout\n");
		destroy_descriptor_layouts(context, layouts);

		return Result::DEVICE_ERROR;
	}

	layouts.material_layout = material_layout;

	// Create global descriptor set layout.

	// Empty for now.
	// constexpr Array<VkDescriptorSetLayoutBinding, 0> global_bindings = {{
	// }};

	VkDescriptorSetLayoutCreateInfo global_layout_info = {};
	global_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	global_layout_info.bindingCount = 0;
	global_layout_info.pBindings = nullptr;

	VkDescriptorSetLayout global_layout = VK_NULL_HANDLE;

	if (vkCreateDescriptorSetLayout(context.logical_device, &global_layout_info, nullptr, &global_layout) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create global descriptor set layout\n");
		destroy_descriptor_layouts(context, layouts);

		return Result::DEVICE_ERROR;
	}

	layouts.global_layout = global_layout;

	return Result::SUCCESS;
}

void
blk::destroy_descriptor_layouts(const Context& context, Descriptor_Layouts& layouts)
{
	vkDestroyDescriptorSetLayout(context.logical_device, layouts.frame_layout, nullptr);
	vkDestroyDescriptorSetLayout(context.logical_device, layouts.material_layout, nullptr);
	vkDestroyDescriptorSetLayout(context.logical_device, layouts.global_layout, nullptr);

	// Destroying the pool frees every descriptor set allocated from it.
	vkDestroyDescriptorPool(context.logical_device, layouts.pool, nullptr);

	layouts = {};
}
