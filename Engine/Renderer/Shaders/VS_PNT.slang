// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Shared.slang"

[[vk::binding(0, 0)]]
ConstantBuffer<Camera_UBO> camera_ubo;

[[vk::push_constant]]
ConstantBuffer<Mesh_Constants> mesh_constants;

[shader("vertex")]
VS_PNT_Output
vs_main(VS_PNT_Input input)
{
	float4 fragment_position = mul(mesh_constants.model, float4(input.position, 1.0));

	VS_PNT_Output output;
	output.position = mul(camera_ubo.projection, mul(camera_ubo.view, fragment_position));
	output.normal = mul(mesh_constants.normal_matrix, float4(input.normal, 0.0)).xyz;
	output.texture_coord = input.texture_coord;
	output.fragment_position = fragment_position.xyz;

	return output;
}


