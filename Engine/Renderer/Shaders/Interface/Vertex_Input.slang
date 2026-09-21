// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

/// Vertex attributes read by the mesh pipeline.
/// The members should match `get_vertex_uv_input_attribute_descriptions` in `Helpers.cpp`, in both order and format.
struct VS_Mesh_Input
{
	float3 position;
	float3 normal;
	float3 tangent;
	float3 bitangent;
	float2 texture_coord;
};
