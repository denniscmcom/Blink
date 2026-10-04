// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/World/Chunk.hpp"

namespace blk
{
struct Mesh;
struct Vector3;
enum class Result;
struct World;

/// Function signature implemented by `Game/` to spawn terrain and chunk entities as descendants of `chunk_root`, with
/// transforms relative to it.
using Spawn_Chunk_Fn = void (*)(World& world, Pool_Handle<Node> chunk_root, Chunk_Coord coord);

struct Stream_Settings
{
	/// Distance from the focus chunk, in chunks, within which chunks are loaded.
	int32_t load_radius = 4;
	/// Distance from the focus chunk, in chunks, beyond which loaded chunks are removed.
	///
	/// Must be greater than `load_radius`. The gap is hysteresis: a chunk loads when it comes within `load_radius` and
	/// stays until it is past `unload_radius`, so moving back and forth across a chunk border does not load and unload
	/// the same row of chunks every step.
	int32_t unload_radius = 5;
	/// When the focus chunk gets this many chunks from the origin, the origin moves to it.
	/// keeps float positions small to avoid rounding errors.
	int32_t rebase_distance = 8;
	/// Maximum number of chunks spawned per `update_streamer` call.
	///
	/// Must be greater than zero. Spreads the spawn cost of a chunk over several frames. Nearest chunks load first, so
	/// while the budget is spent the missing chunks are the ones farthest out, and they load on later calls.
	int32_t max_loads_per_update = 4;
	/// Function pointer to spawn chunk entities.
	Spawn_Chunk_Fn generate;
};
}  // namespace blk
