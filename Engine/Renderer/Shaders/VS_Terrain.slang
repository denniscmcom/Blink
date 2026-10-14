// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Interface/Global_Set.slang"
#include "Interface/Terrain_Constants.slang"
#include "Interface/Vertex_Input.slang"
#include "Pass/Mesh.slang"
#include "Library/Terrain.slang"

[shader("vertex")]
VS_Mesh_Output
vs_main(VS_Vertex_UV input)
{
	// Get heightmap dimensions.
	uint width = 0;
	uint height = 0;
	terrain_heightmap.GetDimensions(width, height);
	int2 heightmap_size = int2(width, height);

	// Get vertex position in the unit flat grid and compute UV.
	float2 grid = input.position.xz;
	float2 uv = terrain_constants.uv_offset + grid * terrain_constants.uv_scale;

	// Compute texel.
	//
	// Rounding to the nearest texel makes neighbouring patches (chunks) read the same texels along their shared edge,
	// ensuring there is no gap between the patches.
	int2 texel = int2(round(uv * float2(heightmap_size)));

	// Texture rows count down but +Z points up seen from above, so the row is flipped to lay the image unmirrored on the
	// ground. Flipping the texel index instead of `v` maps `v = 0` to the last row, the same way `u = 0` maps to the
	// first column.
	texel.y = heightmap_size.y - 1 - texel.y;

	// Load terrain height.
	float vertex_height = load_height(terrain_heightmap, texel, heightmap_size, terrain_ubo.terrain_height);

	// We have vertex height, however we need to compute the slopes for later.
	// To achieve that, we get the height of the four neighbouring vertices. LOD `i` places a vertex every `2^i` texels,
	// so sampling one texel away would give a coarse chunk the normals of detail its triangles do not show.
	//
	// Following the same logic as before when computing the current texel, we flip forward and back. Image rows count
	// downward but the engine convention is +Z forward (up seen from above).

	int vertex_step = 1 << terrain_constants.lod;

	float height_left =
		load_height(terrain_heightmap, texel - int2(vertex_step, 0), heightmap_size, terrain_ubo.terrain_height);
	float height_right =
		load_height(terrain_heightmap, texel + int2(vertex_step, 0), heightmap_size, terrain_ubo.terrain_height);
	float height_back =
		load_height(terrain_heightmap, texel + int2(0, vertex_step), heightmap_size, terrain_ubo.terrain_height);
	float height_forward =
		load_height(terrain_heightmap, texel - int2(0, vertex_step), heightmap_size, terrain_ubo.terrain_height);

	// In other to compute the slopes, we need the vertex spacing in meters.
	float2 vertex_spacing = vertex_step * terrain_ubo.chunk_size / (terrain_constants.uv_scale * float2(heightmap_size));

	// Now we compute the slopes.

	float slope_x = (height_right - height_left) / (2.0f * vertex_spacing.x);
	float slope_z = (height_forward - height_back) / (2.0f * vertex_spacing.y);

	// With the slopes, we can compute the tangent, bitangent and normal of the vertex.
	// The tangent and bitangent follow the surface along X and Z, and their cross product is the normal. Terrain nodes
	// are only translated, so these are already in world space.

	float3 tangent = normalize(float3(1.0f, slope_x, 0.0f));
	float3 bitangent = normalize(float3(0.0f, slope_z, 1.0f));
	float3 normal = normalize(float3(-slope_x, 1.0f, -slope_z));

	// The grid's border wall, whose vertices sit at y = -1, hangs below the chunk's edges as a skirt covering the
	// cracks between chunks of different LODs. It keeps the edge's normal so it is lit like the terrain above it.
	vertex_height += input.position.y * terrain_ubo.skirt_depth;

	// Compute vertex position in model space: where it sits in meters, relative to the chunk's corner.
	float3 local_position = float3(grid.x * terrain_ubo.chunk_size, vertex_height, grid.y * terrain_ubo.chunk_size);

	// Now, we can compute the vertex position in world space.
	float4 fragment_position = mul(terrain_constants.model, float4(local_position, 1.0f));

	// How many chunk units there from the terrain's corner to our vertex, in meters.
	float2 chunk_units = uv / terrain_constants.uv_scale * terrain_ubo.chunk_size;

	VS_Mesh_Output output = {};
	output.position = mul(camera_ubo.projection, mul(camera_ubo.view, fragment_position));
	output.normal = normal;
	output.tangent = tangent;
	output.bitangent = bitangent;

	// The material tiles by the position inside the whole terrain, in meters. The world position would make it slide
	// every time the streamer rebases the origin.
	output.texture_coord = chunk_units / MATERIAL_TILE_SIZE;

	output.fragment_position = fragment_position.xyz;

	return output;
}
