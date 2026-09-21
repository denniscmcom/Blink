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

[[vk::binding(0, 1)]]
ConstantBuffer<Camera_UBO> camera_ubo;

[[vk::binding(1, 1)]]
ConstantBuffer<Light_UBO> light_ubo;

[[vk::binding(2, 1)]]
ConstantBuffer<Skybox_UBO> skybox_ubo;
