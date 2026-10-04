// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

/// Per-draw mesh constants.
/// It should match the host-side `Mesh_Constants` structure and the push constant range in `Renderer.cpp`.
struct Mesh_Constants
{
	column_major float4x4 model;
	column_major float4x4 normal_matrix;
};

[[vk::push_constant]]
ConstantBuffer<Mesh_Constants> mesh_constants;
