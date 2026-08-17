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

namespace
{
blk::Texture_Device find_or_transfer_texture(
	const blk::Context& context,
	blk::Arena& arena,
	blk::Pool_Handle<blk::Texture> handle,
	VkFormat format
);
}  // namespace

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
	material_device.albedo = find_or_transfer_texture(context, arena, material->albedo, VK_FORMAT_R8G8B8A8_SRGB);
	material_device.normal = find_or_transfer_texture(context, arena, material->normal, VK_FORMAT_R8G8B8A8_UNORM);
	material_device.orm = find_or_transfer_texture(context, arena, material->orm, VK_FORMAT_R8G8B8A8_UNORM);

	material_device.uniform_buffer = create_buffer(
		context,
		sizeof(Material_UBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	map_buffer(context, material_device.uniform_buffer);

	Material_UBO material_ubo = {};
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

	VkDescriptorImageInfo albedo_image_info = {};
	albedo_image_info.sampler = context.sampler;
	albedo_image_info.imageView = material_device.albedo.image.view;
	albedo_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo normal_image_info = {};
	normal_image_info.sampler = context.sampler;
	normal_image_info.imageView = material_device.normal.image.view;
	normal_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo orm_image_info = {};
	orm_image_info.sampler = context.sampler;
	orm_image_info.imageView = material_device.orm.image.view;
	orm_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorBufferInfo material_buffer_info = {};
	material_buffer_info.buffer = material_device.uniform_buffer.buffer;
	material_buffer_info.range = material_device.uniform_buffer.size;

	const std::array<VkWriteDescriptorSet, 4> descriptor_writes = {{
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = material_device.descriptor_set,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &albedo_image_info,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = material_device.descriptor_set,
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &normal_image_info,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = material_device.descriptor_set,
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &orm_image_info,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = material_device.descriptor_set,
			.dstBinding = 3,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &material_buffer_info,
		},
	}};

	vkUpdateDescriptorSets(context.logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
	arena.materials.insert({handle, material_device});

	return material_device;
}

namespace
{
blk::Texture_Device
find_or_transfer_texture(
	const blk::Context& context,
	blk::Arena& arena,
	blk::Pool_Handle<blk::Texture> handle,
	VkFormat format
)
{
	if (const std::optional<blk::Texture_Device> texture = find_texture_device(arena, handle))
	{
		return texture.value();
	}

	if (const std::optional<blk::Texture_Device> texture = transfer_texture(context, arena, handle, format))
	{
		return texture.value();
	}

	// FIXME: Create fallback textures at renderer creation. Use them when a mesh is missing a texture.
	BLK_FATAL("Missing texture\n");
	// return {};
}
}  // namespace
