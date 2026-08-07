// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Math/Vector.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Resource/Resource_Storage.hpp"
#include "Engine/Resource/Texture.hpp"

#include <stdint.h>
#include <vector>

namespace blk
{
struct Vertex_PNT
{
	Vector3 position;
	Vector3 normal;
	Vector2 texture_coord;
};

struct Vertex_PNC
{
	Vector3 position;
	Vector3 normal;
	Color_RGB<float> color;
};

using Index = uint32_t;

struct Mesh
{
	Resource_Metadata metadata;
	std::vector<Vertex_PNT> vertices;
	std::vector<Index> indices;
};

constexpr Magic MESH_MAGIC = {'M', 'E', 'S', 'H'};
constexpr uint8_t MESH_VERSION = 1;

Pool_Handle<Mesh> load_mesh(const char* stem);
Pool_Handle<Mesh> compute_uv_sphere(float radius, uint32_t segment_count, uint32_t ring_count);
void unload_mesh(Pool_Handle<Mesh> handle);
Mesh* get_mesh(Pool_Handle<Mesh> handle);
}  // namespace blk
