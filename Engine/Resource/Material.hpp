#pragma once

#include "Core/Pool.hpp"
#include "Resource/Resource_Storage.hpp"

namespace blk
{
struct Texture;

struct Material
{
	Resource_Metadata metadata;
	Pool_Handle<Texture> diffuse_map;
	Pool_Handle<Texture> specular_map;
	float shininess;
};

Pool_Handle<Material> load_material(const char* stem);
void unload_material(Pool_Handle<Material> handle);
Material* get_material(Pool_Handle<Material> handle);
}  // namespace blk
