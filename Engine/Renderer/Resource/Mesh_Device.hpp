// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"

#include <stdint.h>

namespace blk
{
struct Context;
struct Arena;
struct Mesh;
enum class Result;

/// A device mesh.
struct Mesh_Device
{
	/// Host mesh handle.
	Pool_Handle<Mesh> host_handle;
	/// The position of the mesh's first vertex in `Arena::vertex_buffer`, counted in vertices — not bytes — because
	/// that is what `vkCmdDrawIndexed` expects.
	uint32_t first_vertex;
	/// Amount of vertices in the mesh.
	uint32_t vertex_count;
	/// The position of the mesh's first index in `Arena::index_buffer`, counted in indices — not bytes — because that
	/// is what `vkCmdDrawIndexed` expects.
	uint32_t first_index;
	/// Amount of indices in the mesh.
	uint32_t index_count;
};

/// Transfer a host `Mesh` into device.
///
/// It checks for duplicated meshes before transferring. If a mesh already exists in device, it reuses it.
Result transfer_mesh(const Context& context, Arena& arena, const Pool_Handle<Mesh>& host_handle, Mesh_Device& mesh);
/// Unloads a `mesh` from device.
// TODO (Bug): not implemented.
void unload_mesh_from_device(const Context& context, Arena& arena, Mesh_Device& mesh);
}  // namespace blk
