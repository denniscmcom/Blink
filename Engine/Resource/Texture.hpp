// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Array.hpp"
#include "Engine/Core/Math/Color.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Resource/Storage.hpp"

namespace blk
{
/// A deserialized texture.
struct Texture
{
	/// Texture's metadata.
	Resource_Metadata metadata;
	/// Array of pixels. Capacity should be `width` * `height`.
	Dyn_Array<Color_RGBA<uint8_t>> pixels;
	/// Texture's width.
	uint32_t width;
	/// Texture's height.
	uint32_t height;
};

/// Blink's texture magic number. It is found after `BLINK_MAGIC` in `.btexture` binary files.
constexpr char TEXTURE_MAGIC[4] = {'T', 'E', 'X', 'T'};
/// The current implementation version of `.btexture` files.
constexpr uint8_t TEXTURE_VERSION = 1;

/// Creates the storage for textures.
Result create_texture_storage(Allocator* allocator);
/// Destroys the storage for textures.
void destroy_texture_storage();
/// Loads a `.btexture` file with `hash` into memory.
/// @note It is used by `load_material` to load each texture by hash.
Pool_Handle<Texture> load_texture(uint64_t hash);
/// Loads a `Texture` created at runtime into storage.
Pool_Handle<Texture> load_texture(const Texture& texture);
/// Unloads a texture from memory.
void unload_texture(Pool_Handle<Texture> handle);
/// Gets a pointer to a texture's data.
Texture* get_texture(Pool_Handle<Texture> handle);
}  // namespace blk
