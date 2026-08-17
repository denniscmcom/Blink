// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Shared.slang"

struct VS_Main_Input
{
	float3 position;
	float3 normal;
	float3 tangent;
	float3 bitangent;
	float2 texture_coord;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Camera_UBO> camera_ubo;

[[vk::push_constant]]
ConstantBuffer<Mesh_Constants> mesh_constants;

[shader("vertex")]
VS_Main_Output
vs_main(VS_Main_Input input)
{
	float4 fragment_position = mul(mesh_constants.model, float4(input.position, 1.0));

	VS_Main_Output output = {};
	output.position = mul(camera_ubo.projection, mul(camera_ubo.view, fragment_position));
	output.normal = mul(mesh_constants.normal_matrix, float4(input.normal, 0.0)).xyz;
	output.tangent = mul(mesh_constants.model, float4(input.tangent, 0.0)).xyz;
	output.bitangent = mul(mesh_constants.model, float4(input.bitangent, 0.0)).xyz;
	output.texture_coord = input.texture_coord;
	output.fragment_position = fragment_position.xyz;

	return output;
}


