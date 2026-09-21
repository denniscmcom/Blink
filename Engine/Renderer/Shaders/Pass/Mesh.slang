// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

/// Data `VS_Mesh.slang` passes to `FS_Mesh.slang`.
struct VS_Mesh_Output
{
	float4 position : SV_Position;
	float3 normal;
	float3 tangent;
	float3 bitangent;
	float2 texture_coord;
	float3 fragment_position;
};
