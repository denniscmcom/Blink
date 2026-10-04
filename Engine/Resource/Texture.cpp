// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Texture.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Core/String.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Storage.hpp"

namespace
{
/// In-memory storage for textures.
blk::Resource_Storage<blk::Texture> storage = {};
}  // namespace

blk::Result
blk::create_texture_storage(Allocator* allocator)
{
	return create_resource_storage(storage, allocator);
}

void
blk::destroy_texture_storage()
{
	// TODO (Bug): We are leaking `Texture::pixels`.
	// It is a dynamic array created by `load_texture`, so we should destroy it before destroying `storage`.
	// I have not fixed this yet because it is not really that important. We only destroy `storage` on application exit,
	// so in reality we are not leaking anything.
	destroy_resource_storage(storage);
}

blk::Pool_Handle<blk::Texture>
blk::load_texture(const char* stem)
{
	const Pool_Handle<Texture> handle = load_texture(get_resource_hash(stem, Resource_Type::TEXTURE));

	// Loading by hash has no stem to store, so we store it here. This also names a texture a material already loaded.

	if (Texture* texture = get_texture(handle))
	{
		BLK_IF_NOT_SNPRINTF(texture->metadata.stem, MAX_RESOURCE_STEM_SIZE, "%s", stem)
		{
			BLK_ERROR("Invalid texture stem\n");
		}
	}

	return handle;
}

blk::Pool_Handle<blk::Texture>
blk::load_texture(uint64_t hash)
{
	if (is_resource_loaded(storage, hash))
	{
		// If the texture is already loaded, we return its handle.
		return get_resource_handle(storage, hash);
	}

	Serial serial = {};

	// `deserialize_resource` allocates `serial.buffer` and handle ownership to us, so we need to free it.
	BLK_IF_NOT_SUCCESS(deserialize_resource(storage.pool.allocator, Resource_Type::TEXTURE, hash, serial))
	{
		BLK_ERROR("Failed to deserialize texture `%llu`\n", hash);

		return {};
	}

	// Read texture format.
	Texture_Format format = {};

	BLK_IF_NOT_SUCCESS(read(serial, format))
	{
		BLK_ERROR("Failed to read texture format\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	// Read texture size.
	uint32_t width = 0;

	BLK_IF_NOT_SUCCESS(read(serial, width))
	{
		BLK_ERROR("Failed to read texture width\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	uint32_t height = 0;

	BLK_IF_NOT_SUCCESS(read(serial, height))
	{
		BLK_ERROR("Failed to read texture height\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	// Create texture.
	Texture texture = {};
	texture.format = format;
	texture.width = width;
	texture.height = height;

	// Create dynamic array to store pixel data. `width` and `height` come from the file, so we compute the byte count
	// in 64 bits to avoid overflowing.
	const uint64_t byte_count = static_cast<uint64_t>(width) * height * get_pixel_size(format);

	BLK_IF_NOT_SUCCESS(create_dyn_array(texture.pixels, storage.pool.allocator, byte_count))
	{
		BLK_ERROR("Failed to create dynamic array to store texture pixel data\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	for (uint64_t i = 0; i < byte_count; i++)
	{
		// Read pixel byte.
		uint8_t byte = 0;

		BLK_IF_NOT_SUCCESS(read(serial, byte))
		{
			BLK_ERROR("Failed to read texture byte #%llu\n", i);
			destroy_dyn_array(texture.pixels);
			free(*storage.pool.allocator, serial.buffer);

			return {};
		}

		push(texture.pixels, byte);
	}

	// We have to free `serial.buffer` because it is allocated by `load_resource`.
	free(*storage.pool.allocator, serial.buffer);

	// A `.bmaterial` only stores its textures' hashes, so there is no filename to pass here. The stem stays empty
	// unless the texture is loaded by stem.
	return store_resource(storage, texture, hash, "");
}

blk::Pool_Handle<blk::Texture>
blk::load_texture(const Texture& texture)
{
	// We do not need hash nor stem when loading textures created at runtime.
	return store_resource(storage, texture, 0, "");
}

void
blk::unload_texture(const Pool_Handle<Texture> handle)
{
	// First, we have to destroy the pixels array created by `load_texture`.
	Texture* texture = get_texture(handle);

	if (!texture)
	{
		// If `texture` does not exists we have nothing to unload.
		return;
	}

	destroy_dyn_array(texture->pixels);

	// And finally we release the texture from `storage`.
	release_resource(storage, handle);
}

blk::Texture*
blk::get_texture(const Pool_Handle<Texture> handle)
{
	return get_resource(storage, handle);
}

size_t
blk::get_pixel_size(const Texture_Format format)
{
	switch (format)
	{
	case Texture_Format::RGBA8:
		return sizeof(Color_RGBA<uint8_t>);
	case Texture_Format::R16:
		return sizeof(uint16_t);
	}

	BLK_ERROR("Unknown texture format\n");

	return 0;
}
