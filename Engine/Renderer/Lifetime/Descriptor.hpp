#pragma once

#include <vulkan/vulkan.h>

namespace blk
{
struct Context;

struct Descriptor_Layouts
{
	VkDescriptorPool pool = VK_NULL_HANDLE;
	VkDescriptorSetLayout camera_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout light_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout material_layout = VK_NULL_HANDLE;
};

Descriptor_Layouts create_descriptor_layouts(const Context& context);
}  // namespace blk
