// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

/// Meters covered by one repetition of the material textures.
static const float MATERIAL_TILE_SIZE = 32.0f;

/// Returns the height in meters of the heightmap at `texel`, clamped to the heightmap edges.
/// @param terrain_height Meters of a heightmap value of 1.
float load_height(Sampler2D terrain_heightmap, int2 texel, int2 heightmap_size, float terrain_height);

float
load_height(Sampler2D terrain_heightmap, int2 texel, int2 heightmap_size, float terrain_height)
{
	int2 clamped_texel = clamp(texel, int2(0), heightmap_size - 1);

	// `Load` reads the exact texel, so it needs no filtering support from the `R16_UNORM` format.
	// We are using a mip level 0 for now. TODO (Feature): It may change in the future.
	return terrain_heightmap.Load(int3(clamped_texel, 0)).r * terrain_height;
}
