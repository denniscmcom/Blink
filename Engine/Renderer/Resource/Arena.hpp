// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Renderer/Lifetime/Buffer.hpp"
#include "Engine/Renderer/Resource/Material_Device.hpp"
#include "Engine/Renderer/Resource/Mesh_Device.hpp"
#include "Engine/Renderer/Resource/Texture_Device.hpp"

namespace blk
{
struct Material;
struct Texture;
struct Mesh;

/// Maximum vertices supported in scene.
///
/// We use this value to preallocate a big enough vertex buffer.
constexpr size_t MAX_VERTEX_COUNT = 10'000;
/// Maximum indices supported in scene.
///
/// We use this value to preallocate a big enough index buffer.
constexpr size_t MAX_INDEX_COUNT = 10'000;
/// Width of a supported texture.
constexpr size_t TEXTURE_WIDTH = 1'024;
/// Height of a supported texture.
constexpr size_t TEXTURE_HEIGHT = 1'024;
/// Maximum material supported in scene.
///
/// We use this value to calculate descriptor counts.
constexpr size_t MAX_MATERIAL_COUNT = 5;

/// Arena to manage the lifetime of device resources.
///
/// We try to avoid individual buffer allocations. Instead, we preallocate a big enough chunk when creating the
/// renderer and slice it.
struct Arena
{
	/// Host-device vertex buffer pair.
	Host_Device_Buffer vertex_buffer;
	/// Host-device index buffer pair.
	Host_Device_Buffer index_buffer;

	/// Staging buffer to create a texture.
	///
	/// We use this buffer to upload pixel data to, then we create a `Image`, and finally we copy the data from the
	/// buffer to the image. We only need one buffer big enough to fit one texture, since the data is owned by the
	/// image.
	Buffer texture_buffer;

	/// Byte offset in `vertex_buffer` for next write.
	uint32_t vertex_buffer_offset;
	/// Byte offset in `index_buffer` for next write.
	uint32_t index_buffer_offset;

	/// Associates a host mesh handle to a device mesh.
	Hash_Map<Pool_Handle<Mesh>, Mesh_Device> meshes;
	/// Associates a host texture handle to a device texture.
	Hash_Map<Pool_Handle<Texture>, Texture_Device> textures;
	/// Associates a host material handle to a device material.
	Hash_Map<Pool_Handle<Material>, Material_Device> materials;
};

/// Creates `arena`.
Result create_arena(const Context& context, Arena& arena);
/// Destroys `arena`.
// TODO (Bug): not implemented.
void destroy_arena(const Context& context, Arena& arena);
}  // namespace blk
