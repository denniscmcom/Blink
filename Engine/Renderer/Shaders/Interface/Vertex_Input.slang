// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

/// It should match the host-side `Vertex_UV` struct.
struct VS_Vertex_UV
{
	float3 position;
	float3 normal;
	float3 tangent;
	float3 bitangent;
	float2 texture_coord;
};

/// It should match the host-side `Vertex_Spatial` struct.
struct VS_Vertex_Spatial
{
	float3 position;
};
