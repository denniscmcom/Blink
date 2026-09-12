// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Material.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Resource/Storage.hpp"
#include "Engine/Resource/Texture.hpp"

namespace
{
// In-memory storage for materials.
blk::Resource_Storage<blk::Material> storage = {};
}  // namespace

blk::Result
blk::create_material_storage(Allocator* allocator)
{
	return create_resource_storage(storage, allocator);
}

void
blk::destroy_material_storage()
{
	destroy_resource_storage(storage);
}

blk::Pool_Handle<blk::Material>
blk::load_material(const char* stem)
{
	// Compute the material hash.
	const uint64_t hash = get_resource_hash(stem, Resource_Type::MATERIAL);

	if (is_resource_loaded(storage, hash))
	{
		// If the material is already loaded, we return its handle.
		return get_resource_handle(storage, hash);
	}

	Serial serial = {};

	// `deserialize_resource` allocates `serial.buffer` and handle ownership to us, so we need to free it.
	BLK_IF_NOT_SUCCESS(deserialize_resource(storage.pool.allocator, Resource_Type::MATERIAL, hash, serial))
	{
		BLK_ERROR("Failed to deserialize material `%s`\n", stem);

		return {};
	}

	// Get material textures hashes.

	uint64_t albedo_texture_hash = 0;

	BLK_IF_NOT_SUCCESS(read(serial, albedo_texture_hash))
	{
		BLK_ERROR("Failed to read albedo texture hash\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	uint64_t normal_texture_hash = 0;

	BLK_IF_NOT_SUCCESS(read(serial, normal_texture_hash))
	{
		BLK_ERROR("Failed to read normal texture hash\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	uint64_t orm_texture_hash = 0;

	BLK_IF_NOT_SUCCESS(read(serial, orm_texture_hash))
	{
		BLK_ERROR("Failed to read ORM texture hash\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	// Create the material.

	// A failed texture load leaves `POOL_HANDLE_NONE` in the material, which only shows up later as a blank surface, so
	// we report it here instead.

	Material material = {};

	material.albedo = load_texture(albedo_texture_hash);

	if (material.albedo == POOL_HANDLE_NONE<Texture>)
	{
		BLK_ERROR("Failed to load albedo texture `%llu` of material `%s`\n", albedo_texture_hash, stem);
	}

	material.normal = load_texture(normal_texture_hash);

	if (material.normal == POOL_HANDLE_NONE<Texture>)
	{
		BLK_ERROR("Failed to load normal texture `%llu` of material `%s`\n", normal_texture_hash, stem);
	}

	material.orm = load_texture(orm_texture_hash);

	if (material.orm == POOL_HANDLE_NONE<Texture>)
	{
		BLK_ERROR("Failed to load ORM texture `%llu` of material `%s`\n", orm_texture_hash, stem);
	}

	// We have to free `serial.buffer` because it is allocated by `load_resource`.
	free(*storage.pool.allocator, serial.buffer);

	return store_resource(storage, material, hash, stem);
}

void
blk::unload_material(Pool_Handle<Material> handle)
{
	release_resource(storage, handle);
}

blk::Material*
blk::get_material(Pool_Handle<Material> handle)
{
	return get_resource(storage, handle);
}
