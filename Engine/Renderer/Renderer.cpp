// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Core/Core.hpp"
#include "Engine/Core/Math/Color.hpp"
#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Platform/Window_Internal.hpp"
#include "Engine/Renderer/Helpers.hpp"
#include "Engine/Renderer/Lifetime/Context.hpp"
#include "Engine/Renderer/Lifetime/Descriptor.hpp"
#include "Engine/Renderer/Lifetime/Draw_Command.hpp"
#include "Engine/Renderer/Lifetime/Frame.hpp"
#include "Engine/Renderer/Lifetime/Image.hpp"
#include "Engine/Renderer/Lifetime/Pipeline.hpp"
#include "Engine/Renderer/Lifetime/Swapchain.hpp"
#include "Engine/Renderer/Renderer_Internal.hpp"
#include "Engine/Renderer/Resource/Arena.hpp"
#include "Engine/Renderer/Resource/Material_Device.hpp"
#include "Engine/Renderer/Resource/Mesh_Device.hpp"
#include "Engine/Renderer/Resource/Texture_Device.hpp"
#include "Engine/Renderer/Skybox.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Resource/Texture.hpp"

#include <Windows.h>
#include <tracy/Tracy.hpp>
#include <vulkan/vulkan.h>

namespace
{
/// Renderer data.
struct Renderer
{
	/// Swapchain.
	blk::Swapchain swapchain;

	/// Pipeline to render meshes.
	blk::Pipeline mesh_pipeline;
	/// Pipeline to render terrain.
	blk::Pipeline terrain_pipeline;
	/// Pipeline to render the dynamic skybox.
	blk::Pipeline skybox_pipeline;
	/// Pipeline to compute the skybox transmittance LUT.
	blk::Pipeline skybox_transmittance_pipeline;
	/// Pipeline to compute the skybox multiscattering LUT.
	blk::Pipeline skybox_multiscattering_pipeline;
	/// Pipeline to compute the skybox sky-view LUT.
	blk::Pipeline skybox_sky_view_pipeline;
	/// Pipeline to compute the skybox aerial perspective LUT.
	blk::Pipeline skybox_aerial_pipeline;
	/// Pipeline to visualize mesh triangles.
	blk::Pipeline debug_mesh_triangles_pipeline;
	/// Pipeline to visualize terrain triangles.
	blk::Pipeline debug_terrain_triangles_pipeline;
	/// Pipeline to draw lines.
	blk::Pipeline line_pipeline;

	/// Descriptor layouts.
	blk::Descriptor_Layouts descriptor_layouts;
	/// Single set allocated from `descriptor_layouts.global_layout`.
	VkDescriptorSet global_descriptor_set = VK_NULL_HANDLE;

	/// Image for depth testing.
	blk::Image depth_image;
	/// Image for MSAA sampling.
	blk::Image color_image;

	/// Terrain heightmap `begin_render_frame` wrote into `global_descriptor_set`. Terrain is not drawn without one.
	blk::Pool_Handle<blk::Texture> terrain_heightmap_handle;

	/// Frame data.
	blk::Array<blk::Frame, blk::MAX_FRAMES_IN_FLIGHT> frames;
	/// Current frame index. It goes from 0 to `MAX_FRAMES_IN_FLIGHT - 1`.
	uint32_t frame_index;

	/// Descriptor for the current frame.
	blk::Frame_Descriptor current_frame_descriptor;
};

blk::Allocator allocator = {};
blk::Context context = {};
blk::Arena arena = {};
Renderer renderer = {};

}  // namespace

// TODO (Feature): `width` and `height` are currently not used. Swapchain creation is using surface capabilities to get
// it's size.

blk::Result
blk::create_renderer(size_t width, size_t height)
{
	// ============================================================================
	// Create renderer allocator.
	// ============================================================================

	// This allocator is an arena, so nothing it hands out is ever reclaimed — every `Dyn_Array` grow and every
	// `Hash_Map` rehash abandons its old buffer. We size it generously rather than tightly.
	constexpr size_t RENDERER_ALLOCATOR_CAPACITY = 16 * 1'024 * 1'024;

	if (const Result result = create_arena_allocator(allocator, RENDERER_ALLOCATOR_CAPACITY); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create renderer arena allocator\n");

		return result;
	}

	// ============================================================================
	// Create renderer context.
	// ============================================================================

	// Get application window to create `Context`.

	const Window* window = get_window();

	// This should not fail unless the window is not created properly.
	BLK_CHECK(window);

	context = create_context(&allocator, window->hwnd);

	// ============================================================================
	// Create renderer arena.
	// ============================================================================

	if (const Result result = create_arena(context, arena); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create renderer arena\n");
		destroy_renderer();

		return result;
	}

	// ============================================================================
	// Create renderer components.
	// ============================================================================

	// Create swapchain.

	if (const Result result = create_swapchain(context, renderer.swapchain); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create renderer swapchain\n");
		destroy_renderer();

		return result;
	}

	// Create descriptor layouts.

	if (const Result result = create_descriptor_layouts(context, renderer.descriptor_layouts);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create renderer descriptor layouts\n");
		destroy_renderer();

		return result;
	}

	const Array mesh_pipeline_layouts = {{
		renderer.descriptor_layouts.global_layout,
		renderer.descriptor_layouts.frame_layout,
		renderer.descriptor_layouts.material_layout,
	}};

	const Array frame_pipeline_layouts = {{
		renderer.descriptor_layouts.global_layout,
		renderer.descriptor_layouts.frame_layout,
	}};

	// Create depth image.

	if (const Result result = create_image(
			context,
			renderer.swapchain.extent.width,
			renderer.swapchain.extent.height,
			1,
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			1,
			context.msaa_count,
			renderer.depth_image
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create depth image\n");
		destroy_renderer();

		return result;
	}

	// Create color image.

	if (const Result result = create_image(
			context,
			renderer.swapchain.extent.width,
			renderer.swapchain.extent.height,
			1,
			renderer.swapchain.surface_format.format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1,
			context.msaa_count,
			renderer.color_image
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create color image\n");
		destroy_renderer();

		return result;
	}

	// Allocate global descriptor set.

	VkDescriptorSetAllocateInfo global_descriptor_set_allocate_info = {};
	global_descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	global_descriptor_set_allocate_info.descriptorPool = renderer.descriptor_layouts.pool;
	global_descriptor_set_allocate_info.descriptorSetCount = 1;
	global_descriptor_set_allocate_info.pSetLayouts = &renderer.descriptor_layouts.global_layout;

	if (vkAllocateDescriptorSets(
			context.logical_device,
			&global_descriptor_set_allocate_info,
			&renderer.global_descriptor_set
		) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to allocate global descriptor set\n");
		destroy_renderer();

		return Result::DEVICE_ERROR;
	}

	// Its descriptors are written by `bake_renderer`, once the world settings they come from exist.

	// Create shader modules;

	VkShaderModule fs_mesh = VK_NULL_HANDLE;
	VkShaderModule vs_mesh = VK_NULL_HANDLE;

	VkShaderModule vs_terrain = VK_NULL_HANDLE;

	VkShaderModule vs_skybox = VK_NULL_HANDLE;
	VkShaderModule fs_skybox = VK_NULL_HANDLE;
	VkShaderModule cs_skybox_transmittance = VK_NULL_HANDLE;
	VkShaderModule cs_skybox_multiscattering = VK_NULL_HANDLE;
	VkShaderModule cs_skybox_sky_view = VK_NULL_HANDLE;
	VkShaderModule cs_skybox_aerial = VK_NULL_HANDLE;

	VkShaderModule fs_debug_triangles = VK_NULL_HANDLE;

	VkShaderModule vs_line = VK_NULL_HANDLE;
	VkShaderModule fs_line = VK_NULL_HANDLE;

	if (const Result result = create_shader_module(context, "FS_Mesh", fs_mesh); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `FS_Mesh` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "VS_Mesh", vs_mesh); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `VS_Mesh` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "VS_Terrain", vs_terrain); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `VS_Terrain` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "VS_Skybox", vs_skybox); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `VS_Skybox` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "FS_Skybox", fs_skybox); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `FS_Skybox` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "CS_Skybox_Transmittance", cs_skybox_transmittance);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `CS_Skybox_Transmittance` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "CS_Skybox_Multiscattering", cs_skybox_multiscattering);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `CS_Skybox_Multiscattering` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "CS_Skybox_Sky_View", cs_skybox_sky_view);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `CS_Skybox_Sky_View` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "CS_Skybox_Aerial", cs_skybox_aerial);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `CS_Skybox_Aerial` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "FS_Debug_Triangles", fs_debug_triangles);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `FS_Debug_Triangles` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "VS_Line", vs_line); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `VS_Line` shader module\n");
		destroy_renderer();

		return result;
	}

	if (const Result result = create_shader_module(context, "FS_Line", fs_line); result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create `FS_Line` shader module\n");
		destroy_renderer();

		return result;
	}

	// Initialize pipeline shader stage infos.

	const VkPipelineShaderStageCreateInfo fs_mesh_stage_info =
		get_pipeline_shader_stage_create_info(fs_mesh, VK_SHADER_STAGE_FRAGMENT_BIT, "fs_main");
	const VkPipelineShaderStageCreateInfo vs_mesh_stage_info =
		get_pipeline_shader_stage_create_info(vs_mesh, VK_SHADER_STAGE_VERTEX_BIT, "vs_main");

	const VkPipelineShaderStageCreateInfo vs_terrain_stage_info =
		get_pipeline_shader_stage_create_info(vs_terrain, VK_SHADER_STAGE_VERTEX_BIT, "vs_main");

	const VkPipelineShaderStageCreateInfo vs_skybox_stage_info =
		get_pipeline_shader_stage_create_info(vs_skybox, VK_SHADER_STAGE_VERTEX_BIT, "vs_main");
	const VkPipelineShaderStageCreateInfo fs_skybox_stage_info =
		get_pipeline_shader_stage_create_info(fs_skybox, VK_SHADER_STAGE_FRAGMENT_BIT, "fs_main");
	const VkPipelineShaderStageCreateInfo cs_skybox_transmittance_stage_info =
		get_pipeline_shader_stage_create_info(cs_skybox_transmittance, VK_SHADER_STAGE_COMPUTE_BIT, "cs_main");
	const VkPipelineShaderStageCreateInfo cs_skybox_multiscattering_stage_info =
		get_pipeline_shader_stage_create_info(cs_skybox_multiscattering, VK_SHADER_STAGE_COMPUTE_BIT, "cs_main");
	const VkPipelineShaderStageCreateInfo cs_skybox_sky_view_stage_info =
		get_pipeline_shader_stage_create_info(cs_skybox_sky_view, VK_SHADER_STAGE_COMPUTE_BIT, "cs_main");
	const VkPipelineShaderStageCreateInfo cs_skybox_aerial_stage_info =
		get_pipeline_shader_stage_create_info(cs_skybox_aerial, VK_SHADER_STAGE_COMPUTE_BIT, "cs_main");

	const VkPipelineShaderStageCreateInfo fs_debug_triangles_stage_info =
		get_pipeline_shader_stage_create_info(fs_debug_triangles, VK_SHADER_STAGE_FRAGMENT_BIT, "fs_main");

	const VkPipelineShaderStageCreateInfo vs_line_stage_info =
		get_pipeline_shader_stage_create_info(vs_line, VK_SHADER_STAGE_VERTEX_BIT, "vs_main");
	const VkPipelineShaderStageCreateInfo fs_line_stage_info =
		get_pipeline_shader_stage_create_info(fs_line, VK_SHADER_STAGE_FRAGMENT_BIT, "fs_main");

	const Array mesh_pipeline_shader_stages = {{
		vs_mesh_stage_info,
		fs_mesh_stage_info,
	}};

	// Terrain is shaded like any mesh, so it shares `FS_Mesh`.
	const Array terrain_pipeline_shader_stages = {{
		vs_terrain_stage_info,
		fs_mesh_stage_info,
	}};

	const Array skybox_pipeline_shader_stages = {{
		vs_skybox_stage_info,
		fs_skybox_stage_info,
	}};

	const Array debug_mesh_triangles_pipeline_shader_stages = {{
		vs_mesh_stage_info,
		fs_debug_triangles_stage_info,
	}};

	const Array debug_terrain_triangles_pipeline_shader_stages = {{
		vs_terrain_stage_info,
		fs_debug_triangles_stage_info,
	}};

	const Array line_pipeline_shader_stages = {{
		vs_line_stage_info,
		fs_line_stage_info,
	}};

	// Initialize vertex descriptions.

	const Array<VkVertexInputBindingDescription, 1> vertex_uv_input_descriptions =
		get_vertex_uv_input_binding_descriptions();
	const Array<VkVertexInputAttributeDescription, 5> vertex_uv_attribute_descriptions =
		get_vertex_uv_input_attribute_descriptions();

	const Array<VkVertexInputBindingDescription, 1> vertex_color_input_descriptions =
		get_vertex_color_input_binding_descriptions();
	const Array<VkVertexInputAttributeDescription, 2> vertex_color_attribute_descriptions =
		get_vertex_color_input_attribute_descriptions();

	// Initialize push constants.

	VkPushConstantRange mesh_push_contant_range = {};
	mesh_push_contant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	mesh_push_contant_range.offset = 0;
	mesh_push_contant_range.size = sizeof(Mesh_Constants);

	const Array pipeline_push_constant_ranges = {{
		mesh_push_contant_range,
	}};

	VkPushConstantRange terrain_push_contant_range = {};
	terrain_push_contant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	terrain_push_contant_range.offset = 0;
	terrain_push_contant_range.size = sizeof(Terrain_Constants);

	const Array terrain_pipeline_push_constant_ranges = {{
		terrain_push_contant_range,
	}};

	// Create mesh pipeline.

	if (const Result result = create_graphics_pipeline(
			context,
			renderer.swapchain.surface_format,
			renderer.depth_image.format,
			to_array_view(mesh_pipeline_layouts),
			to_array_view(mesh_pipeline_shader_stages),
			to_array_view(vertex_uv_input_descriptions),
			to_array_view(vertex_uv_attribute_descriptions),
			to_array_view(pipeline_push_constant_ranges),
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			renderer.mesh_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create mesh pipeline\n");
		destroy_renderer();

		return result;
	}

	// Create terrain pipeline.
	// It reads the same `Vertex_UV` grid as a mesh, and only its push constants differ.

	if (const Result result = create_graphics_pipeline(
			context,
			renderer.swapchain.surface_format,
			renderer.depth_image.format,
			to_array_view(mesh_pipeline_layouts),
			to_array_view(terrain_pipeline_shader_stages),
			to_array_view(vertex_uv_input_descriptions),
			to_array_view(vertex_uv_attribute_descriptions),
			to_array_view(terrain_pipeline_push_constant_ranges),
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			renderer.terrain_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create terrain pipeline\n");
		destroy_renderer();

		return result;
	}

	// Create skybox pipeline.
	// We do not need vertex descriptions because we are not passing vertex data.

	// The skybox is drawn last, after the scene has filled the depth buffer, and its fragments sit exactly on the far
	// plane. `VK_COMPARE_OP_LESS_OR_EQUAL` therefore only lets it through where the depth buffer still holds the 1.0
	// clear value, which is where nothing was drawn. It writes no depth of its own.

	if (const Result result = create_graphics_pipeline(
			context,
			renderer.swapchain.surface_format,
			renderer.depth_image.format,
			to_array_view(frame_pipeline_layouts),
			to_array_view(skybox_pipeline_shader_stages),
			{},
			{},
			{},
			VK_TRUE,
			VK_FALSE,
			VK_COMPARE_OP_LESS_OR_EQUAL,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			renderer.skybox_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create skybox pipeline\n");
		destroy_renderer();

		return result;
	}

	// Create skybox transmittance compute pipeline.

	if (const Result result = create_compute_pipeline(
			context,
			to_array_view(frame_pipeline_layouts),
			cs_skybox_transmittance_stage_info,
			{},
			renderer.skybox_transmittance_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create skybox transmittance pipeline\n");
		destroy_renderer();

		return result;
	}

	// Create skybox multiscattering compute pipeline.

	if (const Result result = create_compute_pipeline(
			context,
			to_array_view(frame_pipeline_layouts),
			cs_skybox_multiscattering_stage_info,
			{},
			renderer.skybox_multiscattering_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create skybox multiscattering pipeline\n");
		destroy_renderer();

		return result;
	}

	// Create skybox sky-view compute pipeline.

	if (const Result result = create_compute_pipeline(
			context,
			to_array_view(frame_pipeline_layouts),
			cs_skybox_sky_view_stage_info,
			{},
			renderer.skybox_sky_view_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create skybox sky-view pipeline\n");
		destroy_renderer();

		return result;
	}

	// Create skybox aerial perspective compute pipeline.

	if (const Result result = create_compute_pipeline(
			context,
			to_array_view(frame_pipeline_layouts),
			cs_skybox_aerial_stage_info,
			{},
			renderer.skybox_aerial_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create skybox aerial pipeline\n");
		destroy_renderer();

		return result;
	}

	// Create debug mesh triangles pipeline.

	if (const Result result = create_graphics_pipeline(
			context,
			renderer.swapchain.surface_format,
			renderer.depth_image.format,
			to_array_view(mesh_pipeline_layouts),
			to_array_view(debug_mesh_triangles_pipeline_shader_stages),
			to_array_view(vertex_uv_input_descriptions),
			to_array_view(vertex_uv_attribute_descriptions),
			to_array_view(pipeline_push_constant_ranges),
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			renderer.debug_mesh_triangles_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create debug mesh triangles pipeline\n");
		destroy_renderer();

		return result;
	}

	// Create debug terrain triangles pipeline.

	if (const Result result = create_graphics_pipeline(
			context,
			renderer.swapchain.surface_format,
			renderer.depth_image.format,
			to_array_view(mesh_pipeline_layouts),
			to_array_view(debug_terrain_triangles_pipeline_shader_stages),
			to_array_view(vertex_uv_input_descriptions),
			to_array_view(vertex_uv_attribute_descriptions),
			to_array_view(terrain_pipeline_push_constant_ranges),
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			renderer.debug_terrain_triangles_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create debug terrain triangles pipeline\n");
		destroy_renderer();

		return result;
	}

	// Create line pipeline.

	if (const Result result = create_graphics_pipeline(
			context,
			renderer.swapchain.surface_format,
			renderer.depth_image.format,
			to_array_view(frame_pipeline_layouts),
			to_array_view(line_pipeline_shader_stages),
			to_array_view(vertex_color_input_descriptions),
			to_array_view(vertex_color_attribute_descriptions),
			{},
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS,
			VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
			renderer.line_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create line pipeline\n");
		destroy_renderer();

		return result;
	}

	// Create frame data.

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (const Result result = create_frame(context, renderer.descriptor_layouts, renderer.frames.buffer[i]);
			result != Result::SUCCESS)
		{
			BLK_ERROR("Failed to create frame data #%u\n", i);
			destroy_renderer();

			return result;
		}
	}

	return Result::SUCCESS;
}

void
blk::wait_for_device()
{
	if (context.logical_device == VK_NULL_HANDLE)
	{
		// `create_renderer` never got as far as creating a device, so there is nothing submitted to wait for.
		return;
	}

	if (vkDeviceWaitIdle(context.logical_device) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to wait for the device to be idle\n");
	}
}

void
blk::begin_render_frame(const Frame_Descriptor& descriptor)
{
	renderer.current_frame_descriptor = descriptor;

	// Get frame to render to.
	const Frame& frame = renderer.frames.buffer[renderer.frame_index];

	// Wait before updating UBOs.

	if (vkWaitForFences(context.logical_device, 1, &frame.fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to wait for frame fence\n");

		return;
	}

	// Update camera UBO.

	const Camera_UBO camera_ubo = {
		.view = descriptor.view,
		.projection = descriptor.projection,
		.view_position = {descriptor.view_position},
	};

	update_buffer(frame.camera_buffer, &camera_ubo, sizeof(Camera_UBO), 0);

	// Update skybox UBO.

	Matrix4 inversed_view_projection = init_identity_matrix();

	if (const Matrix4 view_no_translation = init_matrix4(init_matrix3(descriptor.view));
		inverse(descriptor.projection * view_no_translation, inversed_view_projection) != Result::SUCCESS)
	{
		BLK_ERROR("Failed to compute the inverse matrix for the sun\n");
	}

	// Compute sun forward vector.

	const Vector3 sun_rotation = {descriptor.sun_elevation.value, descriptor.sun_azimuth.value, 0.0f};
	const Vector3 sun_forward = to_cartesian(sun_rotation);

	const Skybox_UBO skybox_ubo = {
		.inversed_view_projection = inversed_view_projection,
		.sun_direction = -sun_forward,
		.sun_radius = descriptor.sun_radius,
		.rayleigh_scattering = descriptor.rayleigh_scattering,
		.view_height = descriptor.view_position.y,
		.ozone_absorption = descriptor.ozone_absorption,
		.mie_scattering = descriptor.mie_scattering,
		.sun_intensity = {descriptor.sun_intensity, descriptor.sun_intensity, descriptor.sun_intensity},
		.mie_absorption = descriptor.mie_absorption,
		.ground_albedo = convert_srgb_to_linear(descriptor.ground_albedo),
		.rayleigh_scale = descriptor.rayleigh_scale,
		.mie_scale = descriptor.mie_scale,
		.ozone_mid_point = descriptor.ozone_mid_point,
		.ozone_low_point = descriptor.ozone_low_point,
		.planet_radius = descriptor.planet_radius.value,
		.atmosphere_radius = descriptor.atmosphere_radius.value,
		.mie_asymmetry = descriptor.mie_asymmetry,
	};

	update_buffer(frame.skybox_buffer, &skybox_ubo, sizeof(Skybox_UBO), 0);

	// Update terrain UBO.

	const Terrain_UBO terrain_ubo = {
		.chunk_size = descriptor.terrain_chunk_size,
		.terrain_height = descriptor.terrain_height,
		.skirt_depth = descriptor.skirt_depth,
	};

	update_buffer(frame.terrain_buffer, &terrain_ubo, sizeof(Terrain_UBO), 0);

	// Light UBO is updated by `end_render_frame` because we need to know how many lights there are in the scene.

	// Load terrain heightmap if it's not already loaded.

	if (descriptor.terrain_heightmap_handle != renderer.terrain_heightmap_handle)
	{
		// We need to upload the new terrain heightmap. First, we wait for the devide to stop reading it.

		wait_for_device();

		if (Texture_Device* previous_heightmap_device = get(arena.textures, renderer.terrain_heightmap_handle))
		{
			// Unload old heightmap.

			unload_texture_from_device(context, arena, *previous_heightmap_device);
			renderer.terrain_heightmap_handle = {};
		}

		if (const Texture* heightmap = get_texture(descriptor.terrain_heightmap_handle))
		{
			if (BLK_VERIFY(heightmap->format == Texture_Format::R16))
			{
				// Transfer new heightmap to device.

				Texture_Device heightmap_device = {};
				transfer_texture(
					context,
					arena,
					descriptor.terrain_heightmap_handle,
					VK_FORMAT_R16_UNORM,
					heightmap_device
				);

				VkDescriptorImageInfo heightmap_image_info = {};
				heightmap_image_info.sampler = context.texture_sampler;
				heightmap_image_info.imageView = heightmap_device.image.view;
				heightmap_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				// The binding has to match `Descriptor_Layouts::global_layout`.

				VkWriteDescriptorSet heightmap_write = {};
				heightmap_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				heightmap_write.dstSet = renderer.global_descriptor_set;
				heightmap_write.dstBinding = 0;
				heightmap_write.dstArrayElement = 0;
				heightmap_write.descriptorCount = 1;
				heightmap_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				heightmap_write.pImageInfo = &heightmap_image_info;

				vkUpdateDescriptorSets(context.logical_device, 1, &heightmap_write, 0, nullptr);

				// Assign terrain heightmap.
				renderer.terrain_heightmap_handle = descriptor.terrain_heightmap_handle;
			}
		}
	}

	// Reset fence.

	if (vkResetFences(context.logical_device, 1, &frame.fence) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to reset frame fence\n");

		return;
	}
}

void
blk::end_render_frame()
{
	ZoneScoped;

	// Get current frame.
	Frame& frame = renderer.frames.buffer[renderer.frame_index];

	// Get next available swapchain image index.

	uint32_t image_index = UINT32_MAX;

	// TODO (Bug): swapchain recreation for success-ish result codes.
	if (vkAcquireNextImageKHR(
			context.logical_device,
			renderer.swapchain.swapchain,
			UINT64_MAX,
			frame.semaphore,
			nullptr,
			&image_index
		) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to acquire next image from swapchain\n");

		return;
	}

	// Reset frame command buffer.

	if (vkResetCommandBuffer(frame.command_buffer, 0) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to reset command buffer\n");

		return;
	}

	// Begin frame command buffer to record commands.

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(frame.command_buffer, &begin_info) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to begin command buffer\n");
	}

	// Define frame descriptor sets.
	const Array frame_descriptor_sets = {{
		renderer.global_descriptor_set,
		frame.descriptor_set,
	}};

	// TODO (Bug): the skybox passes below run even when `Atmosphere_Settings::sun_node_handle` does not resolve to a
	// live node. `update_frame` then leaves the sun-dependent `Skybox_UBO` fields zeroed (`inversed_view_projection`,
	// `sun_direction`, `sun_intensity`, `view_height`), so the shaders compute on `normalize(0)` and the device is lost
	// within the first frames. Without a sun, every LUT should be cleared with `clear_skybox_lut` instead of
	// dispatched.

#ifdef BLK_DEBUG_TARGET
	if (renderer.current_frame_descriptor.is_skybox_transmittance_enabled)
	{
#endif
		// Compute the skybox transmittance LUT.

		// Transition skybox transmittance image to write.
		transition_image_layout(
			frame.command_buffer,
			frame.skybox_transmittance_lut.image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			{},
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			frame.skybox_transmittance_lut.mip_level_count
		);

		// Bind skybox transmittance pipeline.
		vkCmdBindPipeline(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			renderer.skybox_transmittance_pipeline.pipeline
		);

		// Bind skybox transmittance descriptor set.
		vkCmdBindDescriptorSets(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			renderer.skybox_transmittance_pipeline.layout,
			0,
			frame_descriptor_sets.capacity,
			frame_descriptor_sets.buffer,
			0,
			nullptr
		);

		// Dispatch.
		// One thread per texel.
		vkCmdDispatch(
			frame.command_buffer,
			SKYBOX_TRANSMITTANCE_LUT_WIDTH / SKYBOX_TRANSMITTANCE_WORKGROUP_SIZE,
			SKYBOX_TRANSMITTANCE_LUT_HEIGHT / SKYBOX_TRANSMITTANCE_WORKGROUP_SIZE,
			1
		);

		// Transition the LUT so that it can be sampled. The multiscattering and sky-view passes sample it from compute,
		// `FS_Skybox.slang` samples it for the sun disk from the fragment stage.
		transition_image_layout(
			frame.command_buffer,
			frame.skybox_transmittance_lut.image,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			frame.skybox_transmittance_lut.mip_level_count
		);
#ifdef BLK_DEBUG_TARGET
	}
	else
	{
		clear_skybox_lut(
			frame.command_buffer,
			frame.skybox_transmittance_lut.image,
			SKYBOX_LUT_CLEAR_COLOR,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
		);
	}
#endif

#ifdef BLK_DEBUG_TARGET
	if (renderer.current_frame_descriptor.is_skybox_multiscattering_enabled)
	{
#endif
		// Compute the skybox multiscattering LUT.
		//
		// It samples the transmittance LUT, so it runs after the pass above. It only depends on the atmosphere
		// material, which does not change yet, so it could be baked once instead of rebuilt every frame.

		// Transition skybox multiscattering image to write.
		transition_image_layout(
			frame.command_buffer,
			frame.skybox_multiscattering_lut.image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			{},
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			frame.skybox_multiscattering_lut.mip_level_count
		);

		// Bind skybox multiscattering pipeline.
		vkCmdBindPipeline(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			renderer.skybox_multiscattering_pipeline.pipeline
		);

		// Bind skybox multiscattering descriptor sets.
		vkCmdBindDescriptorSets(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			renderer.skybox_multiscattering_pipeline.layout,
			0,
			frame_descriptor_sets.capacity,
			frame_descriptor_sets.buffer,
			0,
			nullptr
		);

		// Dispatch.
		// One thread per texel.
		vkCmdDispatch(
			frame.command_buffer,
			SKYBOX_MULTISCATTERING_LUT_WIDTH / SKYBOX_MULTISCATTERING_WORKGROUP_SIZE,
			SKYBOX_MULTISCATTERING_LUT_HEIGHT / SKYBOX_MULTISCATTERING_WORKGROUP_SIZE,
			1
		);

		// Transition the skybox multiscattering LUT so that the sky-view and aerial passes can sample it.
		transition_image_layout(
			frame.command_buffer,
			frame.skybox_multiscattering_lut.image,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			frame.skybox_multiscattering_lut.mip_level_count
		);
#ifdef BLK_DEBUG_TARGET
	}
	else
	{
		clear_skybox_lut(
			frame.command_buffer,
			frame.skybox_multiscattering_lut.image,
			SKYBOX_LUT_CLEAR_COLOR,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
		);
	}
#endif

#ifdef BLK_DEBUG_TARGET
	if (renderer.current_frame_descriptor.is_sky_view_enabled)
	{
#endif
		// Compute skybox sky-view LUT.

		// Transition skybox transmittance image to write.
		transition_image_layout(
			frame.command_buffer,
			frame.skybox_sky_view_lut.image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			{},
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			frame.skybox_sky_view_lut.mip_level_count
		);

		// Bind skybox sky-view pipeline.
		vkCmdBindPipeline(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			renderer.skybox_sky_view_pipeline.pipeline
		);

		// Bind skybox sky-view descriptor sets.

		vkCmdBindDescriptorSets(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			renderer.skybox_sky_view_pipeline.layout,
			0,
			frame_descriptor_sets.capacity,
			frame_descriptor_sets.buffer,
			0,
			nullptr
		);

		// Dispatch.
		// One thread per texel.
		vkCmdDispatch(
			frame.command_buffer,
			SKYBOX_SKY_VIEW_LUT_WIDTH / SKYBOX_SKY_VIEW_WORKGROUP_SIZE,
			SKYBOX_SKY_VIEW_LUT_HEIGHT / SKYBOX_SKY_VIEW_WORKGROUP_SIZE,
			1
		);

		// Transition the skybox sky-view LUT so that it can be sampled.
		transition_image_layout(
			frame.command_buffer,
			frame.skybox_sky_view_lut.image,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			frame.skybox_sky_view_lut.mip_level_count
		);

#ifdef BLK_DEBUG_TARGET
	}
	else
	{
		clear_skybox_lut(
			frame.command_buffer,
			frame.skybox_sky_view_lut.image,
			SKYBOX_LUT_CLEAR_COLOR,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
		);
	}
#endif

#ifdef BLK_DEBUG_TARGET
	if (renderer.current_frame_descriptor.is_skybox_aerial_enabled)
	{
#endif
		// Compute the skybox aerial perspective LUT.
		//
		// It samples the transmittance and multiscattering LUTs, so it runs after both passes above.

		// Transition skybox aerial image to write.
		transition_image_layout(
			frame.command_buffer,
			frame.skybox_aerial_lut.image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			{},
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			frame.skybox_aerial_lut.mip_level_count
		);

		// Bind skybox aerial pipeline.
		vkCmdBindPipeline(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			renderer.skybox_aerial_pipeline.pipeline
		);

		// Bind skybox aerial descriptor sets.
		vkCmdBindDescriptorSets(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			renderer.skybox_aerial_pipeline.layout,
			0,
			frame_descriptor_sets.capacity,
			frame_descriptor_sets.buffer,
			0,
			nullptr
		);

		// Dispatch.
		// One thread per froxel column, which walks every depth slice itself.
		vkCmdDispatch(
			frame.command_buffer,
			SKYBOX_AERIAL_LUT_WIDTH / SKYBOX_AERIAL_WORKGROUP_SIZE,
			SKYBOX_AERIAL_LUT_HEIGHT / SKYBOX_AERIAL_WORKGROUP_SIZE,
			1
		);

		// Transition the skybox aerial LUT so that it can be sampled. Section 5.4 applies it from the fragment stage,
		// over opaque geometry after lighting.
		transition_image_layout(
			frame.command_buffer,
			frame.skybox_aerial_lut.image,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			frame.skybox_aerial_lut.mip_level_count
		);

#ifdef BLK_DEBUG_TARGET
	}
	else
	{
		clear_skybox_lut(
			frame.command_buffer,
			frame.skybox_aerial_lut.image,
			SKYBOX_AERIAL_LUT_CLEAR_COLOR,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
		);
	}
#endif

	// Transition current swapchain image to write.

	transition_image_layout(
		frame.command_buffer,
		renderer.swapchain.images.buffer[image_index],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		{},
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1
	);

	// Transition depth image.

	transition_image_layout(
		frame.command_buffer,
		renderer.depth_image.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		renderer.depth_image.mip_level_count
	);

	// Transition color image.

	transition_image_layout(
		frame.command_buffer,
		renderer.color_image.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1
	);

	// We clear to black and initialize the attachment.

	VkClearValue clear_color = {};
	clear_color.color = {0.0f, 0.0f, 0.0f, 1.0f};

	VkRenderingAttachmentInfo color_attachment_info = {};
	color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	color_attachment_info.imageView = renderer.color_image.view;
	// Should match the current image layout after transition.
	color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_attachment_info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	color_attachment_info.resolveImageView = renderer.swapchain.image_views.buffer[image_index];
	color_attachment_info.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment_info.clearValue = clear_color;

	// We clean the depth stencil and initialize the attachment.

	VkClearValue clear_depth = {};
	clear_depth.depthStencil = {.depth = 1.0f, .stencil = 0};

	VkRenderingAttachmentInfo depth_attachment_info = {};
	depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depth_attachment_info.imageView = renderer.depth_image.view;
	// Should match the current depth image layout after transition.
	depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment_info.clearValue = clear_depth;

	VkRect2D render_area = {};
	render_area.extent = renderer.swapchain.extent;

	// Begin dynamic rendering.

	VkRenderingInfo rendering_info = {};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	rendering_info.renderArea = render_area;
	rendering_info.layerCount = 1;
	rendering_info.colorAttachmentCount = 1;
	rendering_info.pColorAttachments = &color_attachment_info;
	rendering_info.pDepthAttachment = &depth_attachment_info;

	vkCmdBeginRendering(frame.command_buffer, &rendering_info);

	// Set viewport.

	// Vulkan clip space has +Y pointing down while the engine is Y-up. We negate the viewport height instead of baking
	// the flip into the projection matrix, which keeps `Core/Math` graphics API agnostic. A negative height negates the
	// y coordinate in clip space, and requires `y` to point at the lower left corner of the viewport.

	VkViewport viewport = {};
	viewport.y = static_cast<float>(renderer.swapchain.extent.height);
	viewport.width = static_cast<float>(renderer.swapchain.extent.width);
	viewport.height = -static_cast<float>(renderer.swapchain.extent.height);
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(frame.command_buffer, 0, 1, &viewport);

	// Set scissor.

	VkRect2D scissor = {};
	scissor.extent = renderer.swapchain.extent;
	vkCmdSetScissor(frame.command_buffer, 0, 1, &scissor);

	// Bind vertex buffer.

	// No offset – initialized to 0.
	constexpr Array<VkDeviceSize, 1> vertex_buffer_offsets = {};

	vkCmdBindVertexBuffers(
		frame.command_buffer,
		0,
		1,
		&arena.vertex_buffer.device.buffer,
		vertex_buffer_offsets.buffer
	);

	// Bind index buffer.

	static_assert(sizeof(Index) == sizeof(uint32_t), "Index buffer is bound as VK_INDEX_TYPE_UINT32");
	vkCmdBindIndexBuffer(frame.command_buffer, arena.index_buffer.device.buffer, 0, VK_INDEX_TYPE_UINT32);

	// Bind frame descriptor sets.

	vkCmdBindDescriptorSets(
		frame.command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		renderer.mesh_pipeline.layout,
		0,
		frame_descriptor_sets.capacity,
		frame_descriptor_sets.buffer,
		0,
		nullptr
	);

	// Iterate over frame mesh draw commands and emit draw calls.

	for (size_t mesh_draw_command_index = 0; mesh_draw_command_index < frame.mesh_draw_commands.count;
		 ++mesh_draw_command_index)
	{
		const Draw_Command& mesh_draw_command = frame.mesh_draw_commands.buffer[mesh_draw_command_index];

		// Bind pipeline.

		// This should not fail unless something is wrong with the implementation.
		BLK_CHECK(mesh_draw_command.pipeline);
		vkCmdBindPipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_draw_command.pipeline->pipeline);

		// Bind descriptor sets specific for meshes.

		vkCmdBindDescriptorSets(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			mesh_draw_command.pipeline->layout,
			2,
			1,
			&mesh_draw_command.material_device.descriptor_set,
			0,
			nullptr
		);

		// Push `Mesh_Constants`.

		vkCmdPushConstants(
			frame.command_buffer,
			mesh_draw_command.pipeline->layout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(Mesh_Constants),
			&mesh_draw_command.mesh_constants
		);

		vkCmdDrawIndexed(
			frame.command_buffer,
			mesh_draw_command.mesh_device.index_count,
			1,
			mesh_draw_command.mesh_device.first_index,
			static_cast<int32_t>(mesh_draw_command.mesh_device.first_vertex),
			0
		);
	}

	// Emit draw calls to render the terrain.

	if (frame.terrain_draw_commands.count > 0)
	{
		// The terrain pipeline layout has different push constant ranges than the mesh one, so it is not compatible for
		// any set and the sets bound above are disturbed (see "compatible for set N" in
		// `External/Vulkan-Docs-main/chapters/descriptorsets.adoc`). We rebind them, and the debug terrain triangles
		// pipeline shares the same layout definition, so they stay valid for it.

		vkCmdBindDescriptorSets(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			renderer.terrain_pipeline.layout,
			0,
			frame_descriptor_sets.capacity,
			frame_descriptor_sets.buffer,
			0,
			nullptr
		);
	}

	for (size_t terrain_draw_command_index = 0; terrain_draw_command_index < frame.terrain_draw_commands.count;
		 ++terrain_draw_command_index)
	{
		const Draw_Command& terrain_draw_command = frame.terrain_draw_commands.buffer[terrain_draw_command_index];

		// Bind pipeline.

		// This should not fail unless something is wrong with the implementation.
		BLK_CHECK(terrain_draw_command.pipeline);
		vkCmdBindPipeline(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			terrain_draw_command.pipeline->pipeline
		);

		// Bind descriptor sets specific for terrain.

		vkCmdBindDescriptorSets(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			terrain_draw_command.pipeline->layout,
			2,
			1,
			&terrain_draw_command.material_device.descriptor_set,
			0,
			nullptr
		);

		// Push `Terrain_Constants`.

		vkCmdPushConstants(
			frame.command_buffer,
			terrain_draw_command.pipeline->layout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(Terrain_Constants),
			&terrain_draw_command.terrain_constants
		);

		vkCmdDrawIndexed(
			frame.command_buffer,
			terrain_draw_command.mesh_device.index_count,
			1,
			terrain_draw_command.mesh_device.first_index,
			static_cast<int32_t>(terrain_draw_command.mesh_device.first_vertex),
			0
		);
	}

	// Render skybox.

	// Bind pipeline.

	vkCmdBindPipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.skybox_pipeline.pipeline);

	// Bind the skybox descriptor sets.

	const Array skybox_descriptor_sets = {{
		renderer.global_descriptor_set,
		frame.descriptor_set,
	}};

	vkCmdBindDescriptorSets(
		frame.command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		renderer.skybox_pipeline.layout,
		0,
		skybox_descriptor_sets.capacity,
		skybox_descriptor_sets.buffer,
		0,
		nullptr
	);

	// The vertex shader builds a full screen triangle out of `SV_VertexID`, so there is no vertex or index buffer
	// to read from and no push constants to set. The vertex and index buffers bound above are simply ignored.

	vkCmdDraw(frame.command_buffer, 3, 1, 0, 0);

	// Render line buffer.

	if (frame.line_vertex_count > 0)
	{
		// Bind pipeline.
		vkCmdBindPipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.line_pipeline.pipeline);

		// Bind vertex buffer.

		constexpr VkDeviceSize line_vertex_buffer_offset = 0;
		vkCmdBindVertexBuffers(
			frame.command_buffer,
			0,
			1,
			&frame.line_vertex_buffer.buffer,
			&line_vertex_buffer_offset
		);

		// Draw lines.
		vkCmdDraw(frame.command_buffer, frame.line_vertex_count, 1, 0, 0);
	}

	// We draw the `Editor/` UI on top of the scene.
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame.command_buffer);

	// End rendering.

	vkCmdEndRendering(frame.command_buffer);

	// Transition swapchain image to present.

	transition_image_layout(
		frame.command_buffer,
		renderer.swapchain.images.buffer[image_index],
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		{},
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1
	);

	// End frame command buffer.

	if (vkEndCommandBuffer(frame.command_buffer) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to end command buffer\n");

		return;
	}

	// Synchronization chain connecting image acquisition, GPU rendering, and presentation.
	// Run `frame.command_buffer` but wait on `frame.semaphore`. When it is done, signal `render_finished_semaphore`.

	VkSemaphoreSubmitInfo wait_semaphore_submit_info = {};
	wait_semaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_semaphore_submit_info.semaphore = frame.semaphore;
	wait_semaphore_submit_info.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkCommandBufferSubmitInfo command_buffer_submit_info = {};
	command_buffer_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	command_buffer_submit_info.commandBuffer = frame.command_buffer;

	VkSemaphoreSubmitInfo signal_semaphore_submit_info = {};
	signal_semaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signal_semaphore_submit_info.semaphore = renderer.swapchain.render_finished_semaphores.buffer[image_index];
	signal_semaphore_submit_info.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo2 queue_submit_info = {};
	queue_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	queue_submit_info.waitSemaphoreInfoCount = 1;
	queue_submit_info.pWaitSemaphoreInfos = &wait_semaphore_submit_info;
	queue_submit_info.commandBufferInfoCount = 1;
	queue_submit_info.pCommandBufferInfos = &command_buffer_submit_info;
	queue_submit_info.signalSemaphoreInfoCount = 1;
	queue_submit_info.pSignalSemaphoreInfos = &signal_semaphore_submit_info;

	if (vkQueueSubmit2(context.queue, 1, &queue_submit_info, frame.fence) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to submit queue\n");

		return;
	}

	// Present image.

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &renderer.swapchain.render_finished_semaphores.buffer[image_index];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &renderer.swapchain.swapchain;
	present_info.pImageIndices = &image_index;

	if (vkQueuePresentKHR(context.queue, &present_info) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to present queue\n");

		return;
	}

	// Empty draw command of the current frame.

	empty(frame.mesh_draw_commands);
	empty(frame.terrain_draw_commands);

	// Update frame index and reset.

	frame.line_vertex_count = 0;
	renderer.frame_index = (renderer.frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

void
blk::draw_mesh(const Mesh_Descriptor& descriptor)
{
	// A missing material falls back to the default one, but we need a mesh to render.
	if (!BLK_VERIFY(descriptor.mesh_handle != POOL_HANDLE_NONE<Mesh>))
	{
		return;
	}

	// Get current frame.
	Frame& frame = renderer.frames.buffer[renderer.frame_index];

	Draw_Command draw_command = {};
	draw_command.mesh_constants.model = descriptor.world_matrix;

	// Initialize mesh normal matrix.

	if (Matrix4 model_inverse = init_identity_matrix();
		inverse(draw_command.mesh_constants.model, model_inverse) == Result::SUCCESS)
	{
		draw_command.mesh_constants.normal_matrix = transpose(model_inverse);
	}
	else
	{
		BLK_ERROR("Failed to initialize model inverse matrix\n");
	}

	// Set pipeline.

	if (renderer.current_frame_descriptor.is_debug_triangle_enabled)
	{
		draw_command.pipeline = &renderer.debug_mesh_triangles_pipeline;
	}
	else
	{
		draw_command.pipeline = &renderer.mesh_pipeline;
	}

	// Transfer mesh to device.
	transfer_mesh(context, arena, descriptor.mesh_handle, draw_command.mesh_device);

	// Transfer material to device.
	transfer_material(
		context,
		renderer.descriptor_layouts,
		arena,
		descriptor.material_handle,
		draw_command.material_device
	);

	// Push `draw_command` to the current `frame`.
	push(frame.mesh_draw_commands, draw_command);
}

void
blk::draw_terrain_chunk(const Terrain_Chunk_Descriptor& descriptor)
{
	if (!BLK_VERIFY(renderer.terrain_heightmap_handle != POOL_HANDLE_NONE<Texture>) ||
		!BLK_VERIFY(descriptor.grid_handle != POOL_HANDLE_NONE<Mesh>))
	{
		return;
	}

	// Get current frame.
	Frame& frame = renderer.frames.buffer[renderer.frame_index];

	// Initialize draw command.

	Draw_Command draw_command = {};
	draw_command.terrain_constants.model = descriptor.world_matrix;
	draw_command.terrain_constants.uv_offset = descriptor.uv_offset;
	draw_command.terrain_constants.uv_scale = descriptor.uv_scale;
	draw_command.terrain_constants.lod = descriptor.lod;

	// Set pipeline.

	if (renderer.current_frame_descriptor.is_debug_triangle_enabled)
	{
		draw_command.pipeline = &renderer.debug_terrain_triangles_pipeline;
	}
	else
	{
		draw_command.pipeline = &renderer.terrain_pipeline;
	}

	// Transfer grid to device.
	transfer_mesh(context, arena, descriptor.grid_handle, draw_command.mesh_device);

	// Transfer material to device.
	transfer_material(
		context,
		renderer.descriptor_layouts,
		arena,
		descriptor.material_handle,
		draw_command.material_device
	);

	// Push draw command.
	push(frame.terrain_draw_commands, draw_command);
}

void
blk::draw_line(const Vector3& start, const Vector3& end, const Color_RGB<float>& color)
{
	// Get current frame.
	Frame& frame = renderer.frames.buffer[renderer.frame_index];

	if (frame.line_vertex_count + 2 > MAX_LINE_VERTEX_BUFFER_COUNT)
	{
		BLK_ERROR("Line buffer is full\n");
		return;
	}

	const Color_RGB<float> linear_color = convert_srgb_to_linear(color);

	const Array<Vertex_Color, 2> vertices = {{
		{.position = start, .color = linear_color},
		{.position = end, .color = linear_color},
	}};

	update_buffer(
		frame.line_vertex_buffer,
		vertices.buffer,
		sizeof(Vertex_Color) * vertices.capacity,
		sizeof(Vertex_Color) * frame.line_vertex_count
	);

	frame.line_vertex_count += 2;
}

void
blk::destroy_renderer()
{
	// Every resource below is still being read by the frames `render_frame` submitted, so we cannot destroy any of
	// them until the GPU is done with them. `destroy_editor` waits too, because it runs before us and destroys the
	// ImGui backend's own resources.
	wait_for_device();

	// TODO (Bug): we are leaking resources created in `create_renderer`.
	// It is not that important because we only destroy the renderer at exit.
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
	pipeline_info.MSAASamples = context.msaa_count;

	return ImGui_ImplVulkan_InitInfo{
		.ApiVersion = context.api_version,
		.Instance = context.instance,
		.PhysicalDevice = context.physical_device,
		.Device = context.logical_device,
		.QueueFamily = context.queue_family_index,
		.Queue = context.queue,
		.DescriptorPool = VK_NULL_HANDLE,
		.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE,
		.MinImageCount = 2,
		.ImageCount = static_cast<uint32_t>(renderer.swapchain.images.count),
		.PipelineInfoMain = pipeline_info,
		.UseDynamicRendering = true
	};
}
