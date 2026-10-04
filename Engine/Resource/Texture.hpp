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
/// Pixel layout of a `Texture`.
///
/// It only describes how the bytes are laid out. The caller picks the color space when it uploads the texture, e.g.
/// `RGBA8` data is uploaded as `SRGB` for albedo and as `UNORM` for normals.
enum class Texture_Format : uint8_t
{
	/// Four 8-bit channels.
	RGBA8,
	/// One 16-bit channel, e.g. a heightmap.
	///
	/// TODO (Research): We upload it as `VK_FORMAT_R16_UNORM` without checking `vkGetPhysicalDeviceFormatProperties`.
	/// The Vulkan specification does not require `VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT` for it (see the 16-bit table in
	/// `External/Vulkan-Docs-main/chapters/formats.adoc`), but desktop GPUs support it in practice. The guaranteed
	/// alternatives are worse: `R16_SFLOAT` loses precision at the top of the range and `R32_SFLOAT` doubles the
	/// memory. In the future, we may pack the heightmap with other terrain data in the other channels of a
	/// multi-channel format.
	R16,
};

/// Returns the size in bytes of one pixel with `format`.
size_t get_pixel_size(Texture_Format format);

/// A deserialized texture.
struct Texture
{
	/// Texture's metadata.
	Resource_Metadata metadata;
	/// Layout of `pixels`.
	Texture_Format format;
	/// Pixel bytes. Count should be `width` * `height` * `get_pixel_size(format)`.
	Dyn_Array<uint8_t> pixels;
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
/// Loads a `.btexture` file with `stem` into memory.
Pool_Handle<Texture> load_texture(const char* stem);
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
