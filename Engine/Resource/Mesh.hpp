// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Resource/Storage.hpp"

#include <stdint.h>

namespace blk
{
/// An UV vertex.
struct Vertex_UV
{
	/// Vertex's position.
	Vector3 position;
	/// Vertex's normal.
	Vector3 normal;
	/// Vertex's tangent.
	Vector3 tangent;
	/// Vertex's bitangent.
	Vector3 bitangent;
	/// Vertex's UV coordinates.
	Vector2 texture_coord;
};

/// A vertex with a spatial component.
struct Vertex_Spatial
{
	/// Vertex's position.
	Vector3 position;
};

/// A vertex index.
using Index = uint32_t;

/// A deserialized mesh.
struct Mesh
{
	/// Mesh's metadata.
	Resource_Metadata metadata;
	/// Array of vertices.
	Dyn_Array<Vertex_UV> vertices;
	/// Array of indices.
	Dyn_Array<Index> indices;
};

/// Blink's mesh magic number. It is found after `BLINK_MAGIC` in `.bmesh` binary files.
constexpr char MESH_MAGIC[4] = {'M', 'E', 'S', 'H'};
/// The current implementation version of `.bmesh` files.
constexpr uint8_t MESH_VERSION = 1;

/// Creates the storage for meshes.
Result create_mesh_storage(Allocator* allocator);
/// Destroys the storage for meshes.
void destroy_mesh_storage();

/// Loads a `.bmesh` file into memory.
Pool_Handle<Mesh> load_mesh(const char* stem);
/// Unloads a mesh from memory.
void unload_mesh(Pool_Handle<Mesh> handle);
/// Gets a pointer to a mesh's data.
Mesh* get_mesh(Pool_Handle<Mesh> handle);

/// Computes a procedural UV sphere and loads it into memory.
/// @param segment_count Number of vertical slices.
/// @param ring_count Number of horizontal slices.
Pool_Handle<Mesh> compute_uv_sphere(float radius, uint32_t segment_count, uint32_t ring_count);
/// Computes a procedural flat grid on the XZ plane, from (0, 0, 0) to (1, 0, 1), facing +Y, and loads it into memory.
/// Its texture coordinates match its XZ position.
/// @param quad_count Number of quads per side.
Pool_Handle<Mesh> compute_grid(uint32_t quad_count);
}  // namespace blk
