// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Pass/Mesh.slang"

/// Renders each mesh triangle with a random color.
[shader("fragment")]
float4
fs_main(VS_Mesh_Output input, uint primitive_id : SV_PrimitiveID) : SV_Target
{
	// PCG hash: consecutive triangle IDs map to unrelated colors.

	uint state = primitive_id * 747796405u + 2891336453u;
	uint hash = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	hash = (hash >> 22u) ^ hash;

	float3 color = float3(hash & 0xFFu, (hash >> 8u) & 0xFFu, (hash >> 16u) & 0xFFu) / 255.0;

	return float4(color, 1.0);
}
