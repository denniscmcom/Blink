struct Camera_UBO
{
	column_major float4x4 view;
	column_major float4x4 projection;
};

static const uint MAX_LIGHT_COUNT = 16;

struct Light_UBO
{
	uint light_count;
	float ambient_strength;
	float3 view_position;
	float3 light_positions[MAX_LIGHT_COUNT];
	float3 light_colors[MAX_LIGHT_COUNT];
};

struct Mesh_Constants
{
	column_major float4x4 model;
	column_major float4x4 normal_matrix;
};

// FIXME: Think of a better architecture to avoid this because it is fragile.
// Sits after Mesh_Constants in the shared push constant block, so the vertex
// stage (model) and the fragment stage (light_index) never alias.
struct Light_Constants
{
	// Must equal sizeof(Mesh_Constants) (two float4x4 = 128 bytes) so light_index
	// starts right after the vertex stage's Mesh_Constants in the shared push
	// constant block, matching the VkPushConstantRange offset on the CPU.
	[[vk::offset(128)]] uint light_index;
};

// FIXME: This changes per material. Currently, my UBOs change per frame. For stuff that changes frequently I prefer
// to use push constants.
struct Material_UBO
{
	float shininess;
};

struct VS_PNT_Input
{
	float3 position;
	float3 normal;
	float2 texture_coord;
};

struct VS_PNT_Output
{
	float4 position : SV_Position;
	float3 normal;
	float2 texture_coord;
	float3 fragment_position;
};

struct VS_PNC_Input
{
	float3 position;
	float3 normal;
	float3 color;
};

struct VS_PNC_Output
{
	float4 position : SV_Position;
	float3 normal;
	float3 color;
};
