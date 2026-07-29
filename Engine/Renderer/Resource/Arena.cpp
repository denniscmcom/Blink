#include "Renderer/Resource/Arena.hpp"

#include <optional>

std::optional<blk::Mesh_Device>
blk::find_mesh_device(const Arena& arena, const Pool_Handle<Mesh> handle)
{
	if (const auto search = arena.meshes.find(handle); search != arena.meshes.end())
	{
		return std::optional(search->second);
	}

	return std::nullopt;
}

std::optional<blk::Texture_Device>
blk::find_texture_device(const Arena& arena, const Pool_Handle<Texture> handle)
{
	if (const auto search = arena.textures.find(handle); search != arena.textures.end())
	{
		return std::optional(search->second);
	}

	return std::nullopt;
}

std::optional<blk::Material_Device>
blk::find_material_device(const Arena& arena, const Pool_Handle<Material> handle)
{
	if (const auto search = arena.materials.find(handle); search != arena.materials.end())
	{
		return std::optional(search->second);
	}

	return std::nullopt;
}
