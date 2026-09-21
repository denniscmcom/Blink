// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Interface/Push_Constants.slang"
#include "Interface/Vertex_Input.slang"
#include "Pass/Mesh.slang"

[shader("vertex")]
VS_Mesh_Output
vs_main(VS_Mesh_Input input)
{
	float4 fragment_position = mul(mesh_constants.model, float4(input.position, 1.0));

	VS_Mesh_Output output = {};
	output.position = mul(camera_ubo.projection, mul(camera_ubo.view, fragment_position));
	output.normal = mul(mesh_constants.normal_matrix, float4(input.normal, 0.0)).xyz;
	output.tangent = mul(mesh_constants.model, float4(input.tangent, 0.0)).xyz;
	output.bitangent = mul(mesh_constants.model, float4(input.bitangent, 0.0)).xyz;
	output.texture_coord = input.texture_coord;
	output.fragment_position = fragment_position.xyz;

	return output;
}
