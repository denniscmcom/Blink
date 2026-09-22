// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

// Descriptor set 1: one set per frame in flight.
// The bindings below are declared in the same order as `frame_bindings` in `Lifetime/Descriptor.cpp`, and the
// structures should match their host-side counterparts in `Lifetime/Frame.hpp`.

/// Camera uniform buffer object.
/// It should match the host-side `Camera_UBO` structure in `Frame.hpp`.
struct Camera_UBO
{
	column_major float4x4 view;
	column_major float4x4 projection;
	float3 view_position;
};

/// Maximum light supported.
static const uint MAX_LIGHT_COUNT = 16;

/// Light uniform buffer object.
/// It should match the host-side `Light_UBO` structure in `Frame.hpp`.
struct Light_UBO
{
	uint light_count;
	float3 light_positions[MAX_LIGHT_COUNT];
	float3 light_colors[MAX_LIGHT_COUNT];
};

/// Skybox uniform buffer object.
/// It should match the host-side `Skybox_UBO` structure in `Frame.hpp`.
struct Skybox_UBO
{
	column_major float4x4 inversed_view_projection;
	float3 sun_direction;
	float sun_radius;
	float view_height;
};

// Descriptor set 1: individual set created for each frame in flight.
// The bindings below are declared in the same order as `frame_bindings` in `Lifetime/Descriptor.cpp`.

[[vk::binding(0, 1)]]
ConstantBuffer<Camera_UBO> camera_ubo;

[[vk::binding(1, 1)]]
ConstantBuffer<Light_UBO> light_ubo;

[[vk::binding(2, 1)]]
ConstantBuffer<Skybox_UBO> skybox_ubo;

/// The transmittance LUT as a storage image, written by `CS_Skybox_Transmittance.slang`.
/// The format should match the image created in `Lifetime/Frame.cpp`.
[[vk::binding(3, 1)]]
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> skybox_transmittance_storage_image;

/// To sample `skybox_transmittance_storage_image`.
[[vk::binding(4, 1)]]
Sampler2D skybox_transmittance_lut;

/// The skybox multiscattering LUT as a storage image, written by `CS_Skybox_Multiscattering.slang`.
/// The format should match the image created in `Lifetime/Frame.cpp`.
[[vk::binding(5, 1)]]
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> skybox_multiscattering_storage_image;

/// To sample `skybox_multiscattering_storage_image`.
[[vk::binding(6, 1)]]
Sampler2D skybox_multiscattering_lut;

/// The skybox sky-view LUT as a storage image, written by `CS_Skybox_Sky_View.slang`.
/// The format should match the image created in `Lifetime/Frame.cpp`.
[[vk::binding(7, 1)]]
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> skybox_sky_view_storage_image;

/// To sample `skybox_sky_view_storage_image`.
[[vk::binding(8, 1)]]
Sampler2D skybox_sky_view_lut;

/// The skybox aerial LUT as a storage image, written by `CS_Skybox_Aerial.slang`.
/// The format should match the image created in `Lifetime/Frame.cpp`.
[[vk::binding(9, 1)]]
[[vk::image_format("rgba16f")]]
RWTexture3D<float4> skybox_aerial_storage_image;

/// To sample `skybox_aerial_storage_image`.
[[vk::binding(10, 1)]]
Sampler3D skybox_aerial_lut;
