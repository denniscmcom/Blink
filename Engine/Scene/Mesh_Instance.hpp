#pragma once

#include "Core/Pool.hpp"

namespace blk
{
struct Mesh;
struct Material;

struct Mesh_Instance
{
	Pool_Handle<Mesh> mesh_handle;
	Pool_Handle<Material> material_handle;
};
}  // namespace blk
