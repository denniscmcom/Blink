// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Resource/Storage.hpp"

namespace blk
{
struct Texture;
struct Allocator;
enum class Result;

/// A deserialized PBR textured material.
struct Material
{
	/// Material's metadata.
	Resource_Metadata metadata;
	/// Handle of the albedo map.
	Pool_Handle<Texture> albedo;
	/// Handle of the normal map.
	Pool_Handle<Texture> normal;
	/// Handle of the occlusion-roughness-metallic (ORM) map.
	Pool_Handle<Texture> orm;
};

/// Blink's material magic number. It is found after `BLINK_MAGIC` in `.bmaterial` binary files.
constexpr char MATERIAL_MAGIC[4] = {'M', 'A', 'T', 'R'};
/// The current implementation version of `.bmaterial` files.
constexpr uint8_t MATERIAL_VERSION = 1;

/// Creates the storage for materials.
Result create_material_storage(Allocator* allocator);
/// Destroys the storage for materials.
void destroy_material_storage();
/// Loads a `.bmaterial` file named `stem` into memory.
Pool_Handle<Material> load_material(const char* stem);
/// Loads a `Material` created at runtime into storage.
Pool_Handle<Material> load_material(const Material& material);
/// Unloads a material from memory.
void unload_material(Pool_Handle<Material> handle);
/// Gets a pointer to a material's data.
Material* get_material(Pool_Handle<Material> handle);
}  // namespace blk
