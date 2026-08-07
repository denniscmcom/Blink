// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Window_Internal.hpp"
#include "Engine/Renderer/Helpers.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Lifetime/Descriptor.hpp"
#include "Engine/Renderer/Lifetime/Frame.hpp"
#include "Engine/Renderer/Lifetime/Image.hpp"
#include "Engine/Renderer/Lifetime/Pipeline.hpp"
#include "Engine/Renderer/Lifetime/Swapchain.hpp"
#include "Engine/Renderer/Renderer_Internal.hpp"
#include "Engine/Renderer/Resource/Arena.hpp"
#include "Engine/Renderer/Resource/Material_Device.hpp"
#include "Engine/Renderer/Resource/Mesh_Device.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Scene/Light.hpp"
#include "Engine/Scene/Mesh_Instance.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/Scene/Scene_Graph.hpp"
#include "Engine/Scene/Transform.hpp"

#include <Windows.h>
#include <vulkan/vulkan.h>

#include <array>

namespace
{
struct Renderer
{
	blk::Swapchain swapchain;
	blk::Pipeline pnt_mesh_pipeline;
	blk::Pipeline pnt_light_pipeline;
	blk::Descriptor_Layouts descriptor_layouts;

	blk::Image depth_image;

	std::array<blk::Frame, blk::MAX_FRAMES_IN_FLIGHT> frames;
	uint64_t frame_index;
};

blk::Context context = {};
blk::Arena arena = {};
Renderer renderer = {};
}  // namespace

void
blk::create_renderer(unsigned width, unsigned height)
{
	Window* window = get_window();
	BLK_CHECK(window);

	context = create_context(window->hwnd);

	renderer.swapchain = create_swapchain(context);
	renderer.descriptor_layouts = create_descriptor_layouts(context);

	renderer.depth_image = create_image(
		context,
		renderer.swapchain.extent.width,
		renderer.swapchain.extent.height,
		VK_FORMAT_D32_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT
	);

	std::array pipeline_layouts = {
		renderer.descriptor_layouts.camera_layout,
		renderer.descriptor_layouts.light_layout,
		renderer.descriptor_layouts.material_layout,
	};

	const VkShaderModule fs_light = create_shader_module(context, "FS_Light");
	const VkShaderModule fs_mesh = create_shader_module(context, "FS_Mesh");
	const VkShaderModule vs_pnt = create_shader_module(context, "VS_PNT");

	const VkPipelineShaderStageCreateInfo fs_light_stage_info =
		get_pipeline_shader_stage_create_info(fs_light, VK_SHADER_STAGE_FRAGMENT_BIT, "fs_main");
	const VkPipelineShaderStageCreateInfo fs_mesh_stage_info =
		get_pipeline_shader_stage_create_info(fs_mesh, VK_SHADER_STAGE_FRAGMENT_BIT, "fs_main");
	const VkPipelineShaderStageCreateInfo vs_pnt_stage_info =
		get_pipeline_shader_stage_create_info(vs_pnt, VK_SHADER_STAGE_VERTEX_BIT, "vs_main");

	const std::array mesh_pipeline_shader_stages = {
		vs_pnt_stage_info,
		fs_mesh_stage_info,
	};

	const std::array light_pipeline_shader_stages = {
		vs_pnt_stage_info,
		fs_light_stage_info,
	};

	const std::vector<VkVertexInputBindingDescription> vertex_pnt_input_descriptions =
		get_vertex_input_descriptions<Vertex_PNT>();
	const std::vector<VkVertexInputAttributeDescription> vertex_pnt_attribute_descriptions =
		get_vertex_input_attribute_descriptions<Vertex_PNT>();

	VkPushConstantRange mesh_push_contant_range = {};
	mesh_push_contant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	mesh_push_contant_range.offset = 0;
	mesh_push_contant_range.size = sizeof(Mesh_Constants);

	VkPushConstantRange light_push_constant_range = {};
	light_push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	light_push_constant_range.offset = sizeof(Mesh_Constants);
	light_push_constant_range.size = sizeof(Light_Constants);

	const std::array pipeline_push_constant_ranges = {
		mesh_push_contant_range,
		light_push_constant_range,
	};

	renderer.pnt_mesh_pipeline = create_pipeline(
		context,
		renderer.swapchain.surface_format,
		renderer.depth_image.format,
		pipeline_layouts,
		mesh_pipeline_shader_stages,
		vertex_pnt_input_descriptions,
		vertex_pnt_attribute_descriptions,
		pipeline_push_constant_ranges
	);

	renderer.pnt_light_pipeline = create_pipeline(
		context,
		renderer.swapchain.surface_format,
		renderer.depth_image.format,
		pipeline_layouts,
		light_pipeline_shader_stages,
		vertex_pnt_input_descriptions,
		vertex_pnt_attribute_descriptions,
		pipeline_push_constant_ranges
	);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		renderer.frames[i] = create_frame(
			context,
			renderer.descriptor_layouts.pool,
			renderer.descriptor_layouts.camera_layout,
			renderer.descriptor_layouts.light_layout
		);
	}

	arena.vertex_buffer =
		create_host_device_buffer(context, sizeof(Vertex_PNT) * MAX_VERTEX_COUNT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	arena.index_buffer =
		create_host_device_buffer(context, sizeof(Index) * MAX_INDEX_COUNT, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	arena.texture_buffer = create_buffer(
		context,
		sizeof(Color_RGBA<uint8_t>) * MAX_TEXTURE_WIDTH * MAX_TEXTURE_HEIGHT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
}

void
blk::destroy_renderer()
{
}

void
blk::update_frame(const Scene_Graph& scene_graph, const Camera_View& camera_view)
{
	Frame& frame = renderer.frames[renderer.frame_index];
	frame.draw_commands.clear();

	std::vector<Vector3> light_positions = {};
	std::vector<Color_RGB<float>> light_colors = {};
	uint32_t light_count = 0;

	for (const auto& node : scene_graph.nodes)
	{
		Draw_Command draw_command = {};
		draw_command.mesh_constants.model = make_transform_matrix(node.transform);

		if (auto model_inverse = inverse(draw_command.mesh_constants.model))
		{
			draw_command.mesh_constants.normal_matrix = transpose(*model_inverse);
		}
		else
		{
			BLK_FATAL("Failed to compute model inverse matrix\n");
		}

		if (node.mesh_instance)
		{
			const Mesh_Instance mesh_instance = node.mesh_instance.value();
			draw_command.pipeline = &renderer.pnt_mesh_pipeline;

			// FIXME: These if-else checking for resources are duplicated. We could do better.
			if (const std::optional<Mesh_Device> mesh_device = find_mesh_device(arena, mesh_instance.mesh_handle))
			{
				draw_command.mesh_device = mesh_device.value();
			}
			else
			{
				draw_command.mesh_device = transfer_mesh(context, arena, mesh_instance.mesh_handle);
			}

			// FIXME: These if-else checking for resources are duplicated. We could do better.
			if (const std::optional<Material_Device> material_device =
					find_material_device(arena, mesh_instance.material_handle))
			{
				draw_command.material_device = material_device.value();
			}
			else
			{
				draw_command.material_device =
					transfer_material(context, arena, renderer.descriptor_layouts, mesh_instance.material_handle);
			}
		}
		else if (node.point_light)
		{
			const Point_Light point_light = node.point_light.value();
			const Pool_Handle<Mesh> light_uv_sphere_handle = compute_uv_sphere(1.0f, 18, 32);

			draw_command.pipeline = &renderer.pnt_light_pipeline;
			draw_command.light_constants.light_index = light_count;

			// FIXME: These if-else checking for resources are duplicated. We could do better.
			if (const std::optional<Mesh_Device> mesh_device = find_mesh_device(arena, light_uv_sphere_handle))
			{
				draw_command.mesh_device = mesh_device.value();
			}
			else
			{
				draw_command.mesh_device = transfer_mesh(context, arena, light_uv_sphere_handle);
			}

			light_positions.push_back(node.transform.position);
			light_colors.push_back(point_light.color);
			light_count += 1;
		}
		else
		{
			continue;
		}

		frame.draw_commands.push_back(draw_command);
	}

	// FIXME: This should be handled elsewhere, not at this point. I think should be earlier.
	if (light_count > MAX_LIGHT_COUNT)
	{
		BLK_FATAL("Max 16 lights supported\n");
	}

	Camera_UBO camera_ubo = {};
	camera_ubo.view = camera_view.view;
	camera_ubo.projection = camera_view.projection;

	Light_UBO light_ubo = {};
	light_ubo.view_position = camera_view.view_position;
	light_ubo.ambient_strength = 0.1f;
	light_ubo.light_count = light_count;

	for (uint32_t i = 0; i < light_count; i++)
	{
		const Color_RGB<float> linear_color = convert_srgb_to_linear(light_colors[i]);

		light_ubo.light_positions[i] = light_positions[i];
		light_ubo.light_colors[i] = {.x = linear_color.r, .y = linear_color.g, .z = linear_color.b};
	}

	if (vkWaitForFences(context.logical_device, 1, &frame.fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to wait for frame fence\n");
	}

	update_buffer(frame.camera_buffer, &camera_ubo, sizeof(Camera_UBO), 0);
	update_buffer(frame.light_buffer, &light_ubo, sizeof(Light_UBO), 0);

	if (vkResetFences(context.logical_device, 1, &frame.fence) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to reset frame fence\n");
	}
}

void
blk::render_frame()
{
	Frame& frame = renderer.frames[renderer.frame_index];
	uint32_t image_index = ~0;

	// TODO: Swapchain recreation for success-ish result codes.
	if (vkAcquireNextImageKHR(
			context.logical_device,
			renderer.swapchain.swapchain,
			UINT64_MAX,
			frame.semaphore,
			nullptr,
			&image_index
		) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to acquire next image from swapchain\n");
	}

	if (vkResetCommandBuffer(frame.command_buffer, NULL) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to reset command buffer\n");
	}

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(frame.command_buffer, &begin_info) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to begin command buffer\n");
	}

	transition_image_layout(
		frame.command_buffer,
		renderer.swapchain.images[image_index],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		{},
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	transition_image_layout(
		frame.command_buffer,
		renderer.depth_image.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT
	);

	VkClearValue clear_color = {};
	clear_color.color = {0.0f, 0.0f, 0.0f, 1.0f};

	VkRenderingAttachmentInfo color_attachment_info = {};
	color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	color_attachment_info.imageView = renderer.swapchain.image_views[image_index];
	color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment_info.clearValue = clear_color;

	VkClearValue clear_depth = {};
	clear_depth.depthStencil = {.depth = 1.0f, .stencil = 0};

	VkRenderingAttachmentInfo depth_attachment_info = {};
	depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depth_attachment_info.imageView = renderer.depth_image.view;
	depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment_info.clearValue = clear_depth;

	VkRect2D render_area = {};
	render_area.extent = renderer.swapchain.extent;

	VkRenderingInfo rendering_info = {};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	rendering_info.renderArea = render_area;
	rendering_info.layerCount = 1;
	rendering_info.colorAttachmentCount = 1;
	rendering_info.pColorAttachments = &color_attachment_info;
	rendering_info.pDepthAttachment = &depth_attachment_info;

	vkCmdBeginRendering(frame.command_buffer, &rendering_info);

	VkViewport viewport = {};
	viewport.width = static_cast<float>(renderer.swapchain.extent.width);
	viewport.height = static_cast<float>(renderer.swapchain.extent.height);
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(frame.command_buffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent = renderer.swapchain.extent;
	vkCmdSetScissor(frame.command_buffer, 0, 1, &scissor);

	constexpr std::array<VkDeviceSize, 1> vertex_buffer_offsets = {};
	vkCmdBindVertexBuffers(
		frame.command_buffer,
		0,
		1,
		&arena.vertex_buffer.device.buffer,
		vertex_buffer_offsets.data()
	);

	static_assert(sizeof(Index) == sizeof(uint32_t), "Index buffer is bound as VK_INDEX_TYPE_UINT32");
	vkCmdBindIndexBuffer(frame.command_buffer, arena.index_buffer.device.buffer, 0, VK_INDEX_TYPE_UINT32);

	std::array frame_descriptor_sets = {frame.camera_descriptor_set, frame.light_descriptor_set};

	vkCmdBindDescriptorSets(
		frame.command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		renderer.pnt_mesh_pipeline.layout,
		0,
		frame_descriptor_sets.size(),
		frame_descriptor_sets.data(),
		0,
		nullptr
	);

	for (const auto& draw_command : frame.draw_commands)
	{
		BLK_CHECK(draw_command.pipeline);
		vkCmdBindPipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, draw_command.pipeline->pipeline);

		vkCmdPushConstants(
			frame.command_buffer,
			renderer.pnt_mesh_pipeline.layout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(Mesh_Constants),
			&draw_command.mesh_constants
		);

		if (draw_command.material_device)
		{
			vkCmdBindDescriptorSets(
				frame.command_buffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				renderer.pnt_mesh_pipeline.layout,
				2,
				1,
				&draw_command.material_device->descriptor_set,
				0,
				nullptr
			);
		}
		else
		{
			vkCmdPushConstants(
				frame.command_buffer,
				renderer.pnt_mesh_pipeline.layout,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				sizeof(Mesh_Constants),
				sizeof(Light_Constants),
				&draw_command.light_constants.light_index
			);
		}

		vkCmdDrawIndexed(
			frame.command_buffer,
			draw_command.mesh_device.index_count,
			1,
			draw_command.mesh_device.index_buffer_offset,
			static_cast<int32_t>(draw_command.mesh_device.vertex_buffer_offset),
			0
		);
	}

	// TODO: Need it for the editor. Probably do something cleaner.
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame.command_buffer);

	vkCmdEndRendering(frame.command_buffer);

	transition_image_layout(
		frame.command_buffer,
		renderer.swapchain.images[image_index],
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		{},
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	if (vkEndCommandBuffer(frame.command_buffer) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to end command buffer\n");
	}

	VkSemaphoreSubmitInfo wait_semaphore_submit_info = {};
	wait_semaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_semaphore_submit_info.semaphore = frame.semaphore;
	wait_semaphore_submit_info.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkCommandBufferSubmitInfo command_buffer_submit_info = {};
	command_buffer_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	command_buffer_submit_info.commandBuffer = frame.command_buffer;

	VkSemaphoreSubmitInfo signal_semaphore_submit_info = {};
	signal_semaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signal_semaphore_submit_info.semaphore = renderer.swapchain.render_finished_semaphores[image_index];
	signal_semaphore_submit_info.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo2 queue_submit_info = {};
	queue_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	queue_submit_info.waitSemaphoreInfoCount = 1;
	queue_submit_info.pWaitSemaphoreInfos = &wait_semaphore_submit_info;
	queue_submit_info.commandBufferInfoCount = 1;
	queue_submit_info.pCommandBufferInfos = &command_buffer_submit_info;
	queue_submit_info.signalSemaphoreInfoCount = 1;
	queue_submit_info.pSignalSemaphoreInfos = &signal_semaphore_submit_info;

	if (vkQueueSubmit2(context.graphics_queue, 1, &queue_submit_info, frame.fence) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to submit queue\n");
	}

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &renderer.swapchain.render_finished_semaphores[image_index];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &renderer.swapchain.swapchain;
	present_info.pImageIndices = &image_index;

	if (vkQueuePresentKHR(context.graphics_queue, &present_info) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to present queue\n");
	}

	renderer.frame_index = (renderer.frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

ImGui_ImplVulkan_InitInfo
blk::get_renderer_imgui_init_info()
{
	VkPipelineRenderingCreateInfoKHR pipeline_create_info = {};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipeline_create_info.colorAttachmentCount = 1;
	pipeline_create_info.pColorAttachmentFormats = &renderer.swapchain.surface_format.format;
	pipeline_create_info.depthAttachmentFormat = renderer.depth_image.format;

	ImGui_ImplVulkan_PipelineInfo pipeline_info = {};
	pipeline_info.PipelineRenderingCreateInfo = pipeline_create_info;

	return ImGui_ImplVulkan_InitInfo{
		.ApiVersion = context.api_version,
		.Instance = context.instance,
		.PhysicalDevice = context.physical_device,
		.Device = context.logical_device,
		.QueueFamily = context.graphics_queue_family_index,
		.Queue = context.graphics_queue,
		.DescriptorPool = VK_NULL_HANDLE,
		.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE,
		.MinImageCount = 2,
		.ImageCount = static_cast<uint32_t>(renderer.swapchain.images.size()),
		.PipelineInfoMain = pipeline_info,
		.UseDynamicRendering = true
	};
}
