// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"

#include <stdint.h>

namespace blk
{
struct Node;

/// Position of a chunk of the infinite XZ grid, in chunks.
/// The exact point is the south-west corner of the chunk.
struct Chunk_Coord
{
	int32_t x;
	int32_t z;
};

/// Chunk data.
struct Chunk
{
	/// Absolute grid coordinate.
	Chunk_Coord coord;
	/// Handle to its root node.
	Pool_Handle<Node> root_handle;
	/// Handle to its terrain node, none if the chunk is outside the terrain.
	Pool_Handle<Node> terrain_handle;
};
}  // namespace blk
