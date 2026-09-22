// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Math/Color.hpp"
#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Platform/Result.hpp"
#include "Engine/Platform/Window_Internal.hpp"
#include "Engine/Renderer/Helpers.hpp"
#include "Engine/Renderer/Lifetime/Command.hpp"
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
#include "Engine/Renderer/Skybox.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Scene/Graph.hpp"
#include "Engine/Scene/Mesh_Instance.hpp"
#include "Engine/Scene/Node.hpp"
#include "Engine/Scene/Point_Light.hpp"
#include "Engine/Scene/Transform.hpp"
#include "Engine/World/World.hpp"

#include <Windows.h>
#include <vulkan/vulkan.h>

#include <math.h>

namespace
{
/// Renderer data.
struct Renderer
{
	/// Swapchain.
	blk::Swapchain swapchain;

	/// Pipeline to render meshes.
	blk::Pipeline mesh_pipeline;
	/// Pipeline to render the dynamic skybox.
	blk::Pipeline skybox_pipeline;
	/// Pipeline to compute the skybox transmittance LUT.
	blk::Pipeline skybox_transmittance_pipeline;
	/// Pipeline to compute the skybox multiscattering LUT.
	blk::Pipeline skybox_multiscattering_pipeline;
	/// Pipeline to compute the skybox sky-view LUT.
	blk::Pipeline skybox_sky_view_pipeline;

	/// Descriptor layouts.
	blk::Descriptor_Layouts descriptor_layouts;
	/// Single set allocated from `descriptor_layouts.global_layout`.
	VkDescriptorSet global_descriptor_set = VK_NULL_HANDLE;

	/// Image for depth testing.
	blk::Image depth_image;

	/// Frame data.
	blk::Array<blk::Frame, blk::MAX_FRAMES_IN_FLIGHT> frames;
	/// Current frame index. It goes from 0 to `MAX_FRAMES_IN_FLIGHT - 1`.
	uint32_t frame_index;
};

blk::Allocator allocator = {};
blk::Context context = {};
blk::Arena arena = {};
Renderer renderer = {};

}  // namespace

// TODO (Feature): `rect` is currently not used. Swapchain creation is using surface capabilities to get it's size.

blk::Result
blk::create_renderer(const Rect<unsigned>& /*rect*/)
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
			renderer.depth_image
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create depth image\n");
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

	// No descriptors writes because it is currently empty.

	// Create shader modules;

	VkShaderModule fs_mesh = VK_NULL_HANDLE;
	VkShaderModule vs_mesh = VK_NULL_HANDLE;
	VkShaderModule vs_skybox = VK_NULL_HANDLE;
	VkShaderModule fs_skybox = VK_NULL_HANDLE;
	VkShaderModule cs_skybox_transmittance = VK_NULL_HANDLE;
	VkShaderModule cs_skybox_multiscattering = VK_NULL_HANDLE;
	VkShaderModule cs_skybox_sky_view = VK_NULL_HANDLE;

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

	// Initialize pipeline shader stage infos.

	const VkPipelineShaderStageCreateInfo fs_mesh_stage_info =
		get_pipeline_shader_stage_create_info(fs_mesh, VK_SHADER_STAGE_FRAGMENT_BIT, "fs_main");
	const VkPipelineShaderStageCreateInfo vs_mesh_stage_info =
		get_pipeline_shader_stage_create_info(vs_mesh, VK_SHADER_STAGE_VERTEX_BIT, "vs_main");
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

	const Array mesh_pipeline_shader_stages = {{
		vs_mesh_stage_info,
		fs_mesh_stage_info,
	}};

	const Array skybox_pipeline_shader_stages = {{
		vs_skybox_stage_info,
		fs_skybox_stage_info,
	}};

	// Initialize vertex descriptions.

	const Array<VkVertexInputBindingDescription, 1> vertex_input_descriptions =
		get_vertex_uv_input_binding_descriptions();
	const Array<VkVertexInputAttributeDescription, 5> vertex_attribute_descriptions =
		get_vertex_uv_input_attribute_descriptions();

	// Initialize push constants.

	VkPushConstantRange mesh_push_contant_range = {};
	mesh_push_contant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	mesh_push_contant_range.offset = 0;
	mesh_push_contant_range.size = sizeof(Mesh_Constants);

	const Array pipeline_push_constant_ranges = {{
		mesh_push_contant_range,
	}};

	// Create mesh pipeline.

	if (const Result result = create_graphics_pipeline(
			context,
			renderer.swapchain.surface_format,
			renderer.depth_image.format,
			to_array_view(mesh_pipeline_layouts),
			to_array_view(mesh_pipeline_shader_stages),
			to_array_view(vertex_input_descriptions),
			to_array_view(vertex_attribute_descriptions),
			to_array_view(pipeline_push_constant_ranges),
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS,
			renderer.mesh_pipeline
		);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create mesh pipeline\n");
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

blk::Result
blk::bake_renderer()
{
	// Bake the skybox transmittance LUT.

	// Create transient command buffer.

	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	BLK_SUCCESS_OR_RETURN(create_command_buffer(context, context.transient_command_pool, command_buffer));

	// Record commands.

	BLK_SUCCESS_OR_ERROR_RETURN(begin_one_time_commands(command_buffer), "Failed to begin one time commands\n");

	// Finish recording commands.
	BLK_SUCCESS_OR_ERROR_RETURN(end_one_time_commands(context, command_buffer), "Failed to end one time commands\n");

	// Destroy transient command buffer.
	destroy_command_buffer(context, context.transient_command_pool, command_buffer);

	return Result::SUCCESS;
}

void
blk::wait_renderer_idle()
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
blk::destroy_renderer()
{
	// Every resource below is still being read by the frames `render_frame` submitted, so we cannot destroy any of them
	// until the GPU is done with them. `destroy_editor` waits too, because it runs before us and destroys the ImGui
	// backend's own resources.
	wait_renderer_idle();

	// TODO (Bug): we are leaking resources created in `create_renderer`.
	// It is not that important because we only destroy the renderer at exit.
}

void
blk::update_frame(const Scene_Graph& scene_graph, const Camera_View& camera_view, const World_Settings& settings)
{
	// Empty draw command of the current frame.

	Frame& frame = renderer.frames.buffer[renderer.frame_index];
	empty(frame.mesh_draw_commands);
	empty(frame.skybox_draw_commands);

	// Initialize `Camera_UBO`.

	Camera_UBO camera_ubo = {};
	camera_ubo.view = camera_view.view;
	camera_ubo.projection = camera_view.projection;
	camera_ubo.view_position.vector = camera_view.view_position;

	// Initialize empty `Light_UBO`.

	Light_UBO light_ubo = {};

	// Initialize skybox UBO.

	Skybox_UBO skybox_ubo = {};

	// We only can have one sun in the scene, so we use this variable to keep that invariant and report the possible
	// error.
	bool sun_exists = false;

	// Now, we iterate over all the `Node`s in the scene and compute new draw commands.

	// TODO (Performance): We are iterating the whole node pool, which is a lot bigger than the real live node count.
	// This is because a pool is fragmented by design. Some options to improve performance if it becames the bottleneck:
	// - Maintain an array of pointers to each live element inside the pool that is updated when removing or inserting
	// elements to the pool. This way we could just iterate that array here. However, the tradeoff if that we have to
	// handle this array updates everytime an element is inserted or removed.

	for (size_t node_slot_index = 0; node_slot_index < scene_graph.nodes.capacity; ++node_slot_index)
	{
		const Pool_Slot<Node> node_slot = scene_graph.nodes.slots[node_slot_index];

		if (!node_slot.is_used)
		{
			// If slot is not used, we continue.
			continue;
		}

		// Now, we now this slot has a live node, so we get a pointer to it.
		const Node* node = &node_slot.element;

		// Initialize draw command.

		Draw_Command draw_command = {};

		// Set `Draw_Command::Mesh_Constants`.

		// We compute `Mesh_Constants` for all node types because there are some nodes that have gizmo meshes. They are
		// not really a `Node_Type::MESH_INSTANCE` because the gizmo mesh is only used by the `Editor/`.

		// Initialize mesh transform matrix.
		draw_command.mesh_constants.model = init_transform_matrix(node->transform);

		// Initialize mesh normal matrix.
		if (Matrix4 model_inverse = {}; inverse(draw_command.mesh_constants.model, model_inverse) == Result::SUCCESS)
		{
			draw_command.mesh_constants.normal_matrix = transpose(model_inverse);
		}
		else
		{
			BLK_ERROR("Failed to initialize model inverse matrix\n");
		}

		switch (node->type)
		{
		case Node_Type::MESH_INSTANCE: {
			// Set `Draw_Command::Pipeline`.

			draw_command.pipeline = &renderer.mesh_pipeline;

			// Now, we have to iterate over all mesh handles in `Node::Mesh_Instance` and emit a draw command for each
			// one.

			for (size_t mesh_handle_index = 0; mesh_handle_index < node->mesh_instance.mesh_handles.count;
				 ++mesh_handle_index)
			{
				// Set `Draw_Command::Mesh_Device`.

				const Pool_Handle<Mesh> host_handle_mesh = node->mesh_instance.mesh_handles.buffer[mesh_handle_index];

				if (host_handle_mesh == POOL_HANDLE_NONE<Mesh>)
				{
					continue;
				}

				// Transfer host mesh to device. `transfer_mesh` will handle duplicated meshes correctly.
				BLK_IF_NOT_SUCCESS(transfer_mesh(context, arena, host_handle_mesh, draw_command.mesh_device))
				{
					BLK_ERROR("Failed to transfer host mesh #%u to device\n", mesh_handle_index);
					continue;
				}

				// Set `Draw_Command::Material_Device`.

				// A `Mesh_Instance` has always the same mesh handles than material handles.
				const Pool_Handle<Material> host_material_handle =
					node->mesh_instance.material_handles.buffer[mesh_handle_index];

				BLK_IF_NOT_SUCCESS(transfer_material(
					context,
					renderer.descriptor_layouts,
					arena,
					host_material_handle,
					draw_command.material_device
				))
				{
					BLK_ERROR("Failed to transfer host material #%u to device\n", mesh_handle_index);
					continue;
				}

				// Push `draw_command` to the current `frame`.
				push(frame.mesh_draw_commands, draw_command);
			}
		}
		break;
		case Node_Type::POINT_LIGHT: {
			// Verify if there are more lights in scene than supported.

			if (light_ubo.light_count >= MAX_LIGHT_COUNT)
			{
				BLK_ERROR("Reached maximum lights supported in scene\n");
				continue;
			}

			// A point light contributes no geometry, so it emits no draw command. We only store its position and
			// color in the arrays declared at the start of this function.

			light_ubo.light_positions[light_ubo.light_count].vector = node->transform.position;

			// We have to convert the light color to linear space.
			const Color_RGB<float> linear_color = convert_srgb_to_linear(node->point_light.color);

			light_ubo.light_colors[light_ubo.light_count].vector =
				Vector3{.x = linear_color.r, .y = linear_color.g, .z = linear_color.b};

			// Increase the number of lights counter.
			light_ubo.light_count += 1;
		}
		break;
		case Node_Type::SPATIAL: {
		}
		break;
		case Node_Type::DIRECTIONAL_LIGHT: {
			if (node->directional_light.is_sun)
			{
				if (sun_exists)
				{
					BLK_ERROR("A sun in the scene already exists\n");
					continue;
				}

				// We strip the translation component from the view matrix.

				if (const Matrix4 view_no_translation = init_matrix4(init_matrix3(camera_view.view));
					inverse(camera_view.projection * view_no_translation, skybox_ubo.inversed_view_projection) !=
					Result::SUCCESS)
				{
					BLK_ERROR("Failed to compute the inverse matrix for the sun\n");
					continue;
				}

				sun_exists = true;

				// Set skybox UBO.
				skybox_ubo.view_height = camera_view.view_position.y;

				// Compute sun forward vector.
				const Vector3 forward = to_cartesian(node->transform.rotation);

				// Negate to get the direction towards the sun.
				skybox_ubo.sun_direction = -forward;

				// Convert sun radius to radians.
				skybox_ubo.sun_radius = to_radians(Degrees{node->transform.scale.x});

				// Create draw command.
				draw_command.pipeline = &renderer.skybox_pipeline;
				push(frame.skybox_draw_commands, draw_command);
			}
			else
			{
				// TODO (Feature): normal directional lights are not implemented yet.
			}
		}
		break;
		}
	}

	// Wait before updating UBOs.

	if (vkWaitForFences(context.logical_device, 1, &frame.fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to wait for frame fence\n");

		return;
	}

	// Update camera UBO.

	BLK_IF_NOT_SUCCESS(update_buffer(frame.camera_buffer, &camera_ubo, sizeof(Camera_UBO), 0))
	{
		BLK_ERROR("Failed to update camera UBO\n");

		return;
	}

	// Update light UBO.

	BLK_IF_NOT_SUCCESS(update_buffer(frame.light_buffer, &light_ubo, sizeof(Light_UBO), 0))
	{
		BLK_ERROR("Failed to update light UBO\n");

		return;
	}

	// Update skybox UBO.

	BLK_IF_NOT_SUCCESS(update_buffer(frame.skybox_buffer, &skybox_ubo, sizeof(Skybox_UBO), 0))
	{
		BLK_ERROR("Failed to update skybox UBO\n");

		return;
	}

	// Reset fence.
	if (vkResetFences(context.logical_device, 1, &frame.fence) != VK_SUCCESS)
	{
		BLK_ERROR("Failed to reset frame fence\n");

		return;
	}
}

void
blk::render_frame(const World_Settings& settings)
{
	// Get current frame.
	const Frame& frame = renderer.frames.buffer[renderer.frame_index];

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

	if (settings.enable_skybox_transmittance)
	{
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
			VK_IMAGE_ASPECT_COLOR_BIT
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
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}
	else
	{
		clear_skybox_lut(
			frame.command_buffer,
			frame.skybox_transmittance_lut.image,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
		);
	}

	if (settings.enable_skybox_multiscattering)
	{
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
			VK_IMAGE_ASPECT_COLOR_BIT
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
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}
	else
	{
		clear_skybox_lut(
			frame.command_buffer,
			frame.skybox_multiscattering_lut.image,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
		);
	}

	if (settings.enable_skybox_sky_view)
	{
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
			VK_IMAGE_ASPECT_COLOR_BIT
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
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}
	else
	{
		clear_skybox_lut(frame.command_buffer, frame.skybox_sky_view_lut.image, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
	}

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
		VK_IMAGE_ASPECT_COLOR_BIT
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
		VK_IMAGE_ASPECT_DEPTH_BIT
	);

	// We clear to black and initialize the attachment.

	VkClearValue clear_color = {};
	clear_color.color = {0.0f, 0.0f, 0.0f, 1.0f};

	VkRenderingAttachmentInfo color_attachment_info = {};
	color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	color_attachment_info.imageView = renderer.swapchain.image_views.buffer[image_index];
	// Should match the current image layout after transition.
	color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

	// Iterate over frame skybox draw commands and emit draw calls.

	for (size_t skybox_draw_command_index = 0; skybox_draw_command_index < frame.skybox_draw_commands.count;
		 ++skybox_draw_command_index)
	{
		const Draw_Command& skybox_draw_command = frame.skybox_draw_commands.buffer[skybox_draw_command_index];

		// Bind pipeline.

		// This should not fail unless something is wrong with the implementation.
		BLK_CHECK(skybox_draw_command.pipeline);
		vkCmdBindPipeline(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			skybox_draw_command.pipeline->pipeline
		);

		// Bind the skybox descriptor sets.

		const Array skybox_descriptor_sets = {{
			renderer.global_descriptor_set,
			frame.descriptor_set,
		}};

		vkCmdBindDescriptorSets(
			frame.command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			skybox_draw_command.pipeline->layout,
			0,
			skybox_descriptor_sets.capacity,
			skybox_descriptor_sets.buffer,
			0,
			nullptr
		);

		// The vertex shader builds a full screen triangle out of `SV_VertexID`, so there is no vertex or index buffer
		// to read from and no push constants to set. The vertex and index buffers bound above are simply ignored.

		vkCmdDraw(frame.command_buffer, 3, 1, 0, 0);
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
		VK_IMAGE_ASPECT_COLOR_BIT
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

	// Update frame index.

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
