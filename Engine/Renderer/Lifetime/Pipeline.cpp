// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Lifetime/Pipeline.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Resource/Shader.hpp"

blk::Result
blk::create_shader_module(const Context& context, const char* stem, VkShaderModule& module)
{
	// Load shader into memory and get a pointer to its data.
	const Pool_Handle<Shader> shader_handle = load_shader(stem);
	const Shader* shader = get_shader(shader_handle);

	if (!shader)
	{
		BLK_ERROR("Failed to load shader `%s`\n", stem);

		return Result::INVALID_ARGUMENTS;
	}

	// Create shader module.

	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = shader->size;
	create_info.pCode = reinterpret_cast<const uint32_t*>(shader->buffer);

	if (vkCreateShaderModule(context.logical_device, &create_info, nullptr, &module) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create shader module\n");

		return Result::DEVICE_ERROR;
	}

	return Result::SUCCESS;
}

blk::Result
blk::create_graphics_pipeline(
	const Context& context,
	VkSurfaceFormatKHR surface_format,
	VkFormat depth_format,
	const Array_View<const VkDescriptorSetLayout>& descriptor_layouts,
	const Array_View<const VkPipelineShaderStageCreateInfo>& shader_stages,
	const Array_View<const VkVertexInputBindingDescription>& vertex_binding_descriptions,
	const Array_View<const VkVertexInputAttributeDescription>& vertex_attribute_descriptions,
	const Array_View<const VkPushConstantRange>& push_constant_ranges,
	VkBool32 enable_depth_test,
	VkBool32 enable_depth_write,
	VkCompareOp depth_compare_op,
	Pipeline& pipeline
)
{
	pipeline = {};

	// Vertex input.

	VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {};
	pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = vertex_binding_descriptions.count;
	pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_descriptions.buffer;
	pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_attribute_descriptions.count;
	pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions.buffer;

	// Input assembly.

	VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = {};
	pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Viewport.

	VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {};
	pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipeline_viewport_state_create_info.viewportCount = 1;
	pipeline_viewport_state_create_info.scissorCount = 1;

	// Rasterization.

	VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = {};
	pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	// This value depends on two things outside `Renderer/`, and changing either one breaks it:
	//
	// 1. `Compiler/Mesh.cpp` mirrors the X axis to convert right-handed FBX into our left-handed space, and sets
	//    `handedness_conversion_retain_winding` so `ufbx` does not reverse the indices to compensate. Mirroring one
	//    axis reverses winding, so meshes reach us wound clockwise.
	// 2. `render_frame` flips Y with a negative viewport height, and the rasterizer decides facing from the sign of the
	//    polygon area in framebuffer coordinates — which flips the winding back to counter-clockwise on screen.
	//
	// The two inversions cancel, so counter-clockwise is correct here.
	pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipeline_rasterization_state_create_info.lineWidth = 1.0f;

	// Multisampling.

	VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {};
	pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Depth/stencil.

	VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {};
	pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipeline_depth_stencil_state_create_info.depthTestEnable = enable_depth_test;
	pipeline_depth_stencil_state_create_info.depthWriteEnable = enable_depth_write;
	pipeline_depth_stencil_state_create_info.depthCompareOp = depth_compare_op;

	// Color blend.

	VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = {};
	pipeline_color_blend_attachment_state.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {};
	pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipeline_color_blend_state_create_info.attachmentCount = 1;
	pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;

	// Dynamic states.
	// These are pipeline states that can change once the pipeline is created.

	constexpr Array<VkDynamicState, 2> pipeline_dynamic_states = {{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	}};

	VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info = {};
	pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipeline_dynamic_state_create_info.dynamicStateCount = pipeline_dynamic_states.capacity;
	pipeline_dynamic_state_create_info.pDynamicStates = pipeline_dynamic_states.buffer;

	// Layout.

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = descriptor_layouts.count;
	pipeline_layout_create_info.pSetLayouts = descriptor_layouts.buffer;
	pipeline_layout_create_info.pushConstantRangeCount = push_constant_ranges.count;
	pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.buffer;

	VkPipelineLayout layout = VK_NULL_HANDLE;

	if (vkCreatePipelineLayout(context.logical_device, &pipeline_layout_create_info, nullptr, &layout) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create pipeline layout\n");

		return Result::DEVICE_ERROR;
	}

	pipeline.layout = layout;

	// Rendering.
	// We are using dynamic rendering (Vulkan 1.3).

	VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_create_info.colorAttachmentCount = 1;
	pipeline_rendering_create_info.pColorAttachmentFormats = &surface_format.format;
	pipeline_rendering_create_info.depthAttachmentFormat = depth_format;

	// Create pipeline.

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.pNext = &pipeline_rendering_create_info;
	graphics_pipeline_create_info.stageCount = shader_stages.count;
	graphics_pipeline_create_info.pStages = shader_stages.buffer;
	graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
	graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
	graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
	graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
	graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
	graphics_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
	graphics_pipeline_create_info.layout = layout;
	graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;

	VkPipeline vk_pipeline = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(
			context.logical_device,
			VK_NULL_HANDLE,
			1,
			&graphics_pipeline_create_info,
			nullptr,
			&vk_pipeline
		) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create graphics pipeline\n");
		destroy_pipeline(context, pipeline);

		return Result::DEVICE_ERROR;
	}

	pipeline.pipeline = vk_pipeline;

	return Result::SUCCESS;
}

blk::Result
blk::create_compute_pipeline(
	const Context& context,
	const Array_View<const VkDescriptorSetLayout>& descriptor_layouts,
	const VkPipelineShaderStageCreateInfo& shader_stage,
	const Array_View<const VkPushConstantRange>& push_constant_ranges,
	Pipeline& pipeline
)
{
	pipeline = {};

	// Layout.

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = descriptor_layouts.count;
	pipeline_layout_create_info.pSetLayouts = descriptor_layouts.buffer;
	pipeline_layout_create_info.pushConstantRangeCount = push_constant_ranges.count;
	pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.buffer;

	VkPipelineLayout layout = VK_NULL_HANDLE;

	if (vkCreatePipelineLayout(context.logical_device, &pipeline_layout_create_info, nullptr, &layout) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create compute pipeline layout\n");

		return Result::DEVICE_ERROR;
	}

	pipeline.layout = layout;

	// Create pipeline.
	// A compute pipeline has a single stage and no fixed-function state, so there

	VkComputePipelineCreateInfo compute_pipeline_create_info = {};
	compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.stage = shader_stage;
	compute_pipeline_create_info.layout = layout;

	VkPipeline vk_pipeline = VK_NULL_HANDLE;

	if (vkCreateComputePipelines(
			context.logical_device,
			VK_NULL_HANDLE,
			1,
			&compute_pipeline_create_info,
			nullptr,
			&vk_pipeline
		) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to create compute pipeline\n");
		destroy_pipeline(context, pipeline);

		return Result::DEVICE_ERROR;
	}

	pipeline.pipeline = vk_pipeline;

	return Result::SUCCESS;
}

void
blk::destroy_shader_module(const Context& context, VkShaderModule module)
{
	vkDestroyShaderModule(context.logical_device, module, nullptr);
}

void
blk::destroy_pipeline(const Context& context, Pipeline& pipeline)
{
	vkDestroyPipeline(context.logical_device, pipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(context.logical_device, pipeline.layout, nullptr);

	pipeline = {};
}
