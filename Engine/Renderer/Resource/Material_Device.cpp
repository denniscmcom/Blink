// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Resource/Material_Device.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Lifetime/Descriptor.hpp"
#include "Engine/Renderer/Resource/Arena.hpp"
#include "Engine/Renderer/Resource/Texture_Device.hpp"
#include "Engine/Resource/Material.hpp"

blk::Result
blk::transfer_material(
	const Context& context,
	const Descriptor_Layouts& descriptor_layouts,
	Arena& arena,
	Pool_Handle<Material> host_handle,
	Material_Device& material
)
{
	material = {};

	// Check if `material` is already on device.
	if (const Material_Device* material_ptr = get(arena.materials, host_handle))
	{
		material = *material_ptr;

		return Result::SUCCESS;
	}

	// Get material data.
	const Material* host_material = get_material(host_handle);

	if (!host_material)
	{
		BLK_ERROR("Failed to get host material to transfer\n");

		return Result::INVALID_ARGUMENTS;
	}

	material.host_handle = host_handle;

	// Transfer material textures.

	if (const Result result =
			transfer_texture(context, arena, host_material->albedo, VK_FORMAT_R8G8B8A8_SRGB, material.albedo);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to transfer albedo texture to device\n");
		unload_material_from_device(context, arena, material);

		return Result::DEVICE_ERROR;
	}

	if (const Result result =
			transfer_texture(context, arena, host_material->normal, VK_FORMAT_R8G8B8A8_UNORM, material.normal);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to transfer normal texture to device\n");
		unload_material_from_device(context, arena, material);

		return Result::DEVICE_ERROR;
	}

	if (const Result result =
			transfer_texture(context, arena, host_material->orm, VK_FORMAT_R8G8B8A8_UNORM, material.orm);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to transfer ORM texture to device\n");
		unload_material_from_device(context, arena, material);

		return Result::DEVICE_ERROR;
	}

	// Create material UBO.
	// TODO (Performance): We may want to suballocate from a single buffer like we do with the index and vertex buffer.

	if (const Result result = create_buffer(
			context,
			sizeof(Material_UBO),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			material.uniform_buffer
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create material uniform buffer\n");
		unload_material_from_device(context, arena, material);

		return result;
	}

	if (const Result result = map_buffer(context, material.uniform_buffer); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to map material uniform buffer\n");
		unload_material_from_device(context, arena, material);

		return result;
	}

	// Update material UBO.

	Material_UBO material_ubo = {};

	if (const Result result = update_buffer(material.uniform_buffer, &material_ubo, sizeof(Material_UBO), 0);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to update material uniform buffer\n");
		unload_material_from_device(context, arena, material);

		return result;
	}

	// Allocate a descriptor set for the material.

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = descriptor_layouts.pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = &descriptor_layouts.material_layout;

	if (vkAllocateDescriptorSets(context.logical_device, &descriptor_set_allocate_info, &material.descriptor_set) !=
		VK_SUCCESS)
	{
		BLK_ERROR("Failed to allocate material descriptor set\n");
		unload_material_from_device(context, arena, material);

		return Result::DEVICE_ERROR;
	}

	// Update the descriptor set.

	VkDescriptorBufferInfo material_buffer_info = {};
	material_buffer_info.buffer = material.uniform_buffer.buffer;
	material_buffer_info.range = material.uniform_buffer.size;

	VkDescriptorImageInfo albedo_image_info = {};
	albedo_image_info.sampler = context.texture_sampler;
	albedo_image_info.imageView = material.albedo.image.view;
	albedo_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo normal_image_info = {};
	normal_image_info.sampler = context.texture_sampler;
	normal_image_info.imageView = material.normal.image.view;
	normal_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo orm_image_info = {};
	orm_image_info.sampler = context.texture_sampler;
	orm_image_info.imageView = material.orm.image.view;
	orm_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// The bindings have to match `Descriptor_Layouts::material_layout`.
	const Array<VkWriteDescriptorSet, 4> descriptor_writes = {{
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = material.descriptor_set,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &material_buffer_info,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = material.descriptor_set,
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &albedo_image_info,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = material.descriptor_set,
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &normal_image_info,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = material.descriptor_set,
			.dstBinding = 3,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &orm_image_info,
		},
	}};

	vkUpdateDescriptorSets(context.logical_device, descriptor_writes.capacity, descriptor_writes.buffer, 0, nullptr);

	// Insert the host_handle-material pair into the hash map.
	insert(arena.materials, host_handle, material);

	return Result::SUCCESS;
}

void
blk::unload_material_from_device(const Context& context, Arena& arena, Material_Device& material)
{
}
