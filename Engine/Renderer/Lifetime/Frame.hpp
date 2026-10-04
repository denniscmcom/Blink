// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Math/Matrix.hpp"
#include "Engine/Core/Math/Unit.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Lifetime/Descriptor.hpp"
#include "Engine/Renderer/Lifetime/Image.hpp"

#include <stddef.h>
#include <vulkan/vulkan.h>

namespace blk
{
struct Draw_Command;
struct Context;
struct Pipeline;
enum class Result;

/// A `Vector3` carrying the alignment std140 requires of a `float3` inside a uniform block.
///
/// A three-component vector has a base alignment of four times its scalar alignment, and an array's alignment is
/// rounded up to a multiple of 16, so a `float3` array in a uniform block has a 16-byte stride. `Vector3` is 12 bytes,
/// so an array of them puts every element but the first at an offset the shader does not read from.
///
/// @see https://docs.vulkan.org/spec/latest/chapters/interfaces.html#interfaces-resources-layout
struct alignas(16) Vector3_STD140
{
	Vector3 vector;
};

// Uniform Buffer Objects lives the entire frame.

/// Camera uniform buffer object.
/// It should match the GPU-side `Camera_UBO` structure in `Shaders/Interface/Frame_Set.slang`.
struct Camera_UBO
{
	/// View matrix.
	Matrix4 view;
	/// Projection matrix.
	Matrix4 projection;
	/// View/eye position.
	Vector3_STD140 view_position;
};

static_assert(sizeof(Camera_UBO) == 64 + 64 + 16);
static_assert(offsetof(Camera_UBO, view) == 0);
static_assert(offsetof(Camera_UBO, projection) == 64);
static_assert(offsetof(Camera_UBO, view_position) == 64 + 64);

/// Maximum amount of lights in a scene supported by the renderer.
constexpr uint32_t MAX_LIGHT_COUNT = 16;

/// Light uniform buffer object.
/// It should match the GPU-side `Light_UBO` structure in `Shaders/Interface/Frame_Set.slang`.
struct Light_UBO
{
	/// Amount of lights in the scene.
	uint32_t light_count;
	/// Array of light positions.
	Vector3_STD140 light_positions[MAX_LIGHT_COUNT];
	/// Array of light colors.
	Vector3_STD140 light_colors[MAX_LIGHT_COUNT];
};

static_assert(sizeof(Light_UBO) == 16 + (16 * MAX_LIGHT_COUNT) + (16 * MAX_LIGHT_COUNT));
static_assert(offsetof(Light_UBO, light_count) == 0);
static_assert(offsetof(Light_UBO, light_positions) == 16);
static_assert(offsetof(Light_UBO, light_colors) == 16 + 16 * MAX_LIGHT_COUNT);

/// Skybox uniform buffer object.
/// It should match the GPU-side `Skybox_UBO` structure in `Shaders/Interface/Frame_Set.slang`.
///
/// Every `Vector3` is a plain `Vector3` and not a `Vector3_STD140`, and each one is followed by a scalar. std140 gives
/// a `float3` a base alignment of 16 but a size of 12, and a scalar aligns to 4, so the shader packs that scalar into
/// the four bytes left after the vector. The host struct packs it there too, and every vector lands on a multiple of
/// 16 without padding. A `Vector3_STD140` would push the scalar to the next 16 bytes, where the shader does not read
/// it. `Vector3_STD140` is for arrays, where the 16-byte stride applies to every element.
///
/// Atmosphere parameters are described in `Atmosphere_Settings`.
struct Skybox_UBO
{
	/// The inverse of projection times view with the translation component removed.
	Matrix4 inversed_view_projection;
	/// Normalized direction toward the sun.
	Vector3 sun_direction;
	/// The radius of the sun in radians.
	Radians sun_radius;
	Vector3 rayleigh_scattering;
	/// View height above the planet surface in meters.
	float view_height;
	Vector3 ozone_absorption;
	float mie_scattering;
	/// The sun's `Directional_Light::color` in linear space, scaled by its `Directional_Light::intensity`.
	Vector3 sun_intensity;
	float mie_absorption;
	Vector3 ground_albedo;
	float rayleigh_scale;
	float mie_scale;
	float ozone_mid_point;
	float ozone_low_point;
	float planet_radius;
	float atmosphere_radius;
	float mie_asymmetry;
};

static_assert(sizeof(Skybox_UBO) == 64 + 16 * 5 + 4 * 6);
static_assert(offsetof(Skybox_UBO, inversed_view_projection) == 0);
static_assert(offsetof(Skybox_UBO, sun_direction) == 64);
static_assert(offsetof(Skybox_UBO, sun_radius) == 64 + 12);
static_assert(offsetof(Skybox_UBO, rayleigh_scattering) == 80);
static_assert(offsetof(Skybox_UBO, view_height) == 80 + 12);
static_assert(offsetof(Skybox_UBO, ozone_absorption) == 96);
static_assert(offsetof(Skybox_UBO, mie_scattering) == 96 + 12);
static_assert(offsetof(Skybox_UBO, sun_intensity) == 112);
static_assert(offsetof(Skybox_UBO, mie_absorption) == 112 + 12);
static_assert(offsetof(Skybox_UBO, ground_albedo) == 128);
static_assert(offsetof(Skybox_UBO, rayleigh_scale) == 128 + 12);
static_assert(offsetof(Skybox_UBO, mie_scale) == 144);
static_assert(offsetof(Skybox_UBO, ozone_mid_point) == 148);
static_assert(offsetof(Skybox_UBO, ozone_low_point) == 152);
static_assert(offsetof(Skybox_UBO, planet_radius) == 156);
static_assert(offsetof(Skybox_UBO, atmosphere_radius) == 160);
static_assert(offsetof(Skybox_UBO, mie_asymmetry) == 164);

/// Terrain uniform buffer object.
/// It should match the GPU-side `Terrain_UBO` structure in `Shaders/Interface/Frame_Set.slang`.
struct Terrain_UBO
{
	/// `compute_chunk_size`.
	float chunk_size;
	/// `Terrain_Settings::height`.
	float terrain_height;
};

static_assert(sizeof(Terrain_UBO) == 8);
static_assert(offsetof(Terrain_UBO, chunk_size) == 0);
static_assert(offsetof(Terrain_UBO, terrain_height) == 4);

/// Initial number of draw commands a frame can hold before its arrays have to grow.
constexpr size_t INITIAL_DRAW_COMMAND_COUNT = 64;

/// Data needed by each frame.
struct Frame
{
	/// Frame command buffer to record commands to.
	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	/// To synchronize device-device. The host never sees it.
	/// Coordinates work between queue submissions without involving the host.
	VkSemaphore semaphore = VK_NULL_HANDLE;
	/// To Synchronize device-host. The host waits.
	/// In `update_frame` the host waits until the device has finished executing this frame's previous submission.
	/// Without it, we'd start overwriting `command_buffer` and UBO data while the device is still reading them.
	VkFence fence = VK_NULL_HANDLE;
	/// Camera buffer to store `Camera_UBO`.
	Buffer camera_buffer;
	/// Light buffer to store `Light_UBO`.
	Buffer light_buffer;
	/// Skybox buffer to store `Skybox_UBO`.
	Buffer skybox_buffer;
	/// Terrain buffer to store `Terrain_UBO`.
	Buffer terrain_buffer;
	/// Transmittance LUT for the skybox.
	Image skybox_transmittance_lut;
	/// Multiscattering LUT for the skybox.
	Image skybox_multiscattering_lut;
	/// Sky-view LUT for the skybox.
	Image skybox_sky_view_lut;
	/// Aerial perspective LUT for the skybox.
	Image skybox_aerial_lut;
	/// The `Descriptor_Layouts::pool` the descriptor sets below were allocated from. We keep it so that `destroy_frame`
	/// can return them to it.
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	/// Frame descriptor set allocated from `descriptor_pool`.
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	/// Array of draw commands to render meshes.
	Dyn_Array<Draw_Command> mesh_draw_commands;
	/// Array of draw commands to render terrain.
	Dyn_Array<Draw_Command> terrain_draw_commands;
	/// Array of draw commands to render the skybox.
	Dyn_Array<Draw_Command> skybox_draw_commands;
};

/// Creates `frame` data.
Result create_frame(const Context& context, const Descriptor_Layouts& layouts, Frame& frame);
/// Destroys a `frame`.
void destroy_frame(const Context& context, Frame& frame);
}  // namespace blk
