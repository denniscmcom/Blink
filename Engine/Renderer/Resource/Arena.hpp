#pragma once

#include "Renderer/Lifetime/Buffer.hpp"
#include "Renderer/Lifetime/Image.hpp"
#include "Renderer/Resource/Material_Device.hpp"
#include "Renderer/Resource/Mesh_Device.hpp"
#include "Renderer/Resource/Texture_Device.hpp"

#include <optional>
#include <unordered_map>

namespace blk
{
constexpr size_t MAX_VERTEX_COUNT = 10'000;
constexpr size_t MAX_INDEX_COUNT = 10'000;
constexpr size_t MAX_TEXTURE_COUNT = 5;
constexpr size_t MAX_TEXTURE_WIDTH = 1'024;
constexpr size_t MAX_TEXTURE_HEIGHT = 1'024;
constexpr size_t MAX_MATERIAL_COUNT = 5;

// TODO: Right now it does not handle removing data from buffer nor reusing unused space.
// FIXME: Right now I have a circular dependency (resolved by forward declared). Arena needs all the resources, and each
//   resource needs Arena. I think is better to make Arena a class with private stuff to avoid corruption of state, and
//   handle resource transfer to gpu with methods.
struct Arena
{
	Host_Device_Buffer vertex_buffer;
	Host_Device_Buffer index_buffer;

	Buffer texture_buffer;

	uint32_t vertex_buffer_byte_offset;
	uint32_t index_buffer_byte_offset;

	std::unordered_map<Pool_Handle<Mesh>, Mesh_Device, Pool_Handle_Hash<Mesh>> meshes;
	std::unordered_map<Pool_Handle<Texture>, Texture_Device, Pool_Handle_Hash<Texture>> textures;
	std::unordered_map<Pool_Handle<Material>, Material_Device, Pool_Handle_Hash<Material>> materials;
};

// FIXME: The code inside these functions is kind of duplicated.
std::optional<Mesh_Device> find_mesh_device(const Arena& arena, Pool_Handle<Mesh> handle);
std::optional<Texture_Device> find_texture_device(const Arena& arena, Pool_Handle<Texture> handle);
std::optional<Material_Device> find_material_device(const Arena& arena, Pool_Handle<Material> handle);
}  // namespace blk
