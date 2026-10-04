// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Pool.hpp"

#include <stddef.h>
#include <stdint.h>

namespace blk
{
struct Mesh;
struct Texture;

/// Number of LODs in `Terrain_Settings::chunk_grid_handles`. LOD `i` has `chunk_texel_count >> i` quads per side.
constexpr size_t TERRAIN_LOD_COUNT = 5;

/// Terrain settings shared by every `Node_Type::TERRAIN` node.
///
/// @note `width` and `height` are the values from the terrain definition in Gaea. Gaea places the terrain inside a box,
/// so they do not represent the maximum width or height of the actual terrain, instead they represent the size of that
/// box.
struct Terrain_Settings
{
	/// Array of LOD flat grid of vertices, initialized by `create_world` and displaced by the vertex shader.
	Array<Pool_Handle<Mesh>, TERRAIN_LOD_COUNT> chunk_grid_handles;
	/// Distance from the focus chunk, in chunks, within which a chunk uses LOD `i` or a finer one. Chunks beyond the
	/// last radius use the coarsest LOD.
	///
	/// Must be strictly increasing.
	Array<int32_t, TERRAIN_LOD_COUNT - 1> lod_radii = {1, 2, 3, 4};
	/// Extra distance, in chunks, a chunk must move past `lod_radii` before it switches to a coarser LOD.
	///
	/// Must not be negative. The gap is hysteresis: moving back and forth across a chunk border does not switch the
	/// same chunks between two LODs every step.
	int32_t lod_hysteresis = 1;
	/// A `Texture_Format::R16` heightmap.
	Pool_Handle<Texture> heightmap_handle;
	/// Width of the terrain definition in meters.
	float width;
	/// Height of the terrain definition in meters.
	float height;
	/// Terrain texels per side.
	uint32_t resolution;
	/// Terrain texels per chunk side.
	uint32_t chunk_texel_count = 64;
};

/// Computes the chunk size in meters.
/// Returns zero if the terrain is not configured.
float compute_chunk_size(const Terrain_Settings& settings);
}  // namespace blk
