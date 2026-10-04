// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

/// Per-draw terrain constants.
/// It should match the host-side `Terrain_Constants` structure and the push constant range in `Renderer.cpp`.
struct Terrain_Constants
{
	column_major float4x4 model;
	float2 uv_offset;
	float uv_scale;
};

[[vk::push_constant]]
ConstantBuffer<Terrain_Constants> terrain_constants;
