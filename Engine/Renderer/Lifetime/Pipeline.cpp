// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Pipeline.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Resource/Shader.hpp"

#include <array>

VkShaderModule
blk::create_shader_module(const Context& context, const char* stem)
{
	BLK_DEBUG("Creating shader module...\n");

	const Pool_Handle<Shader> shader_handle = load_shader(stem);
	const Shader* shader = get_shader(shader_handle);

	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = shader->size;
	create_info.pCode = reinterpret_cast<const uint32_t*>(shader->buffer);

	VkShaderModule shader_module;

	if (vkCreateShaderModule(context.logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create shader module\n");
	}

	return shader_module;
}

blk::Pipeline
blk::create_pipeline(
	const Context& context,
	VkSurfaceFormatKHR surface_format,
	VkFormat depth_format,
	std::span<const VkDescriptorSetLayout> descriptor_layouts,
	std::span<const VkPipelineShaderStageCreateInfo> shader_stages,
	std::span<const VkVertexInputBindingDescription> vertex_binding_descriptions,
	std::span<const VkVertexInputAttributeDescription> vertex_attribute_descriptions,
	std::span<const VkPushConstantRange> push_constant_ranges
)
{
	VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {};
	pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = vertex_binding_descriptions.size();
	pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_descriptions.data();
	pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_attribute_descriptions.size();
	pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = {};
	pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {};
	pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipeline_viewport_state_create_info.viewportCount = 1;
	pipeline_viewport_state_create_info.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = {};
	pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipeline_rasterization_state_create_info.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {};
	pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {};
	pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
	pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
	pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;

	VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = {};
	pipeline_color_blend_attachment_state.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {};
	pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipeline_color_blend_state_create_info.attachmentCount = 1;
	pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;

	constexpr std::array pipeline_dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info = {};
	pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipeline_dynamic_state_create_info.dynamicStateCount = pipeline_dynamic_states.size();
	pipeline_dynamic_state_create_info.pDynamicStates = pipeline_dynamic_states.data();

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = descriptor_layouts.size();
	pipeline_layout_create_info.pSetLayouts = descriptor_layouts.data();
	pipeline_layout_create_info.pushConstantRangeCount = push_constant_ranges.size();
	pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();

	VkPipelineLayout layout;

	if (vkCreatePipelineLayout(context.logical_device, &pipeline_layout_create_info, nullptr, &layout) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create pipeline layout\n");
	}

	VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_create_info.colorAttachmentCount = 1;
	pipeline_rendering_create_info.pColorAttachmentFormats = &surface_format.format;
	pipeline_rendering_create_info.depthAttachmentFormat = depth_format;

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.pNext = &pipeline_rendering_create_info;
	graphics_pipeline_create_info.stageCount = shader_stages.size();
	graphics_pipeline_create_info.pStages = shader_stages.data();
	graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
	graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
	graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
	graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
	graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
	graphics_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
	graphics_pipeline_create_info.layout = layout;
	graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;

	VkPipeline pipeline;

	if (vkCreateGraphicsPipelines(
			context.logical_device,
			VK_NULL_HANDLE,
			1,
			&graphics_pipeline_create_info,
			nullptr,
			&pipeline
		) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create graphics pipeline\n");
	}

	return Pipeline{.pipeline = pipeline, .layout = layout};
}
