#pragma once

#include "Core/Math/Vector.hpp"
#include "Core/Pool.hpp"
#include "Resource/Texture.hpp"

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

using Index = uint16_t;

struct Mesh
{
	Resource_Metadata metadata;
	std::vector<Vertex_PNT> vertices;
	std::vector<Index> indices;
};

Pool_Handle<Mesh> load_mesh(const char* stem);
Pool_Handle<Mesh> compute_uv_sphere(float radius, uint32_t segment_count, uint32_t ring_count);
void unload_mesh(Pool_Handle<Mesh> handle);
Mesh* get_mesh(Pool_Handle<Mesh> handle);
}  // namespace blk
