// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Vector.hpp"

namespace blk
{
struct Mesh;

/// A square patch of terrain, displaced on the device by a region of the world's heightmap.
///
/// The heightmap and its height scale are shared by every patch, so they live in `Terrain_Settings`.
///
/// @warning The node must only be translated. The terrain normals are computed in the node's space, so a rotation or a
/// scale would not reach them.
struct Terrain_Ref
{
	/// Corner of the heightmap region the patch covers, in heightmap UV.
	Vector2 uv_offset;
	/// Side length of the heightmap region the patch covers, in heightmap UV.
	float uv_scale;
	/// Current LOD for this terrain.
	uint32_t lod;
};
}  // namespace blk
