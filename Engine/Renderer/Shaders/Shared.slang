// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================


static const double PI = 3.14159265358979323846;

/// Camera uniform buffer object.
struct Camera_UBO
{
	/// View matrix.
	column_major float4x4 view;
	/// Projection matrix.
	column_major float4x4 projection;
	/// Camera position.
	float3 view_position;
};

/// Maximum light supported.
static const uint MAX_LIGHT_COUNT = 16;

/// Light uniform buffer object.
struct Light_UBO
{
	uint light_count;
	float3 light_positions[MAX_LIGHT_COUNT];
	float3 light_colors[MAX_LIGHT_COUNT];
};

struct Mesh_Constants
{
	column_major float4x4 model;
	column_major float4x4 normal_matrix;
};

struct VS_Main_Output
{
	float4 position : SV_Position;
	float3 normal;
	float3 tangent;
	float3 bitangent;
	float2 texture_coord;
	float3 fragment_position;
};
