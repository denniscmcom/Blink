// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Resource/Material_Device.hpp"

#include "Engine/Platform/Log.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Lifetime/Descriptor.hpp"
#include "Engine/Renderer/Resource/Arena.hpp"
#include "Engine/Renderer/Resource/Texture_Device.hpp"
#include "Engine/Resource/Material.hpp"

#include <array>
#include <optional>

blk::Material_Device
blk::transfer_material(
	const Context& context,
	Arena& arena,
	const Descriptor_Layouts& descriptor_layouts,
	const Pool_Handle<Material> handle
)
{
	const Material* material = get_material(handle);

	if (!material)
	{
		BLK_FATAL("Failed to get material\n");
	}

	Material_Device material_device = {};
	material_device.handle = handle;

	// The diffuse map is color, so it goes through an sRGB format and the hardware
	// decodes it to linear on sample. The specular map is an intensity mask rather
	// than color, so it stays UNORM.
	if (const std::optional<Texture_Device> diffuse_map = find_texture_device(arena, material->diffuse_map))
	{
		material_device.diffuse_map = diffuse_map.value();
	}
	else
	{
		material_device.diffuse_map = transfer_texture(context, arena, material->diffuse_map, VK_FORMAT_R8G8B8A8_SRGB);
	}

	if (const std::optional<Texture_Device> specular_map = find_texture_device(arena, material->specular_map))
	{
		material_device.specular_map = specular_map.value();
	}
	else
	{
		material_device.specular_map =
			transfer_texture(context, arena, material->specular_map, VK_FORMAT_R8G8B8A8_UNORM);
	}

	material_device.uniform_buffer = create_buffer(
		context,
		sizeof(Material_UBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	map_buffer(context, material_device.uniform_buffer);

	Material_UBO material_ubo = {};
	material_ubo.shininess = material->shininess;

	update_buffer(material_device.uniform_buffer, &material_ubo, sizeof(Material_UBO), 0);

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = descriptor_layouts.pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = &descriptor_layouts.material_layout;

	if (vkAllocateDescriptorSets(
			context.logical_device,
			&descriptor_set_allocate_info,
			&material_device.descriptor_set
		) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to allocate material descriptor set\n");
	}

	VkDescriptorImageInfo diffuse_image_info = {};
	diffuse_image_info.sampler = context.sampler;
	diffuse_image_info.imageView = material_device.diffuse_map.image.view;
	diffuse_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo specular_image_info = {};
	specular_image_info.sampler = context.sampler;
	specular_image_info.imageView = material_device.specular_map.image.view;
	specular_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorBufferInfo material_buffer_info = {};
	material_buffer_info.buffer = material_device.uniform_buffer.buffer;
	material_buffer_info.range = material_device.uniform_buffer.size;

	const std::array<VkWriteDescriptorSet, 3> descriptor_writes = {{
		{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		 .dstSet = material_device.descriptor_set,
		 .dstBinding = 0,
		 .dstArrayElement = 0,
		 .descriptorCount = 1,
		 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		 .pImageInfo = &diffuse_image_info},
		{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		 .dstSet = material_device.descriptor_set,
		 .dstBinding = 1,
		 .dstArrayElement = 0,
		 .descriptorCount = 1,
		 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		 .pImageInfo = &specular_image_info},
		{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		 .dstSet = material_device.descriptor_set,
		 .dstBinding = 2,
		 .dstArrayElement = 0,
		 .descriptorCount = 1,
		 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		 .pBufferInfo = &material_buffer_info},
	}};

	vkUpdateDescriptorSets(context.logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

	arena.materials.insert({handle, material_device});

	return material_device;
}
