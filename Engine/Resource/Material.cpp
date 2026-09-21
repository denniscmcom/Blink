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
/// In-memory storage for materials.
blk::Resource_Storage<blk::Material> storage = {};

/// Fallback material.
blk::Pool_Handle<blk::Material> fallback_material_handle = {};

/// Fallback texture handle used when a material has no albedo component.
blk::Pool_Handle<blk::Texture> fallback_albedo_handle = {};
/// Fallback texture handle used when a material has no normal component.
blk::Pool_Handle<blk::Texture> fallback_normal_handle = {};
/// Fallback texture handle used when a material has no ORM component.
blk::Pool_Handle<blk::Texture> fallback_orm_handle = {};
}  // namespace

blk::Result
blk::create_material_storage(Allocator* allocator)
{
	BLK_SUCCESS_OR_RETURN(create_resource_storage(storage, allocator));

	// Create fallback textures.

	constexpr uint32_t fallback_texture_width = 1'024;
	constexpr uint32_t fallback_texture_height = 1'024;
	constexpr uint64_t fallback_texture_size = fallback_texture_width * fallback_texture_height;

	constexpr Color_RGBA<uint8_t> fallback_albedo_light_color = {.r = 255, .g = 0, .b = 255, .a = 255};
	constexpr Color_RGBA<uint8_t> fallback_albedo_dark_color = {.r = 105, .g = 0, .b = 105, .a = 255};

	constexpr Color_RGBA<uint8_t> fallback_normal_color = {.r = 128, .g = 128, .b = 255, .a = 255};

	constexpr Color_RGBA<uint8_t> fallback_orm_color = {.r = 255, .g = 255, .b = 0, .a = 255};

	Texture fallback_albedo = {};
	Texture fallback_normal = {};
	Texture fallback_orm = {};

	// Set dimensions.

	fallback_albedo.width = fallback_texture_width;
	fallback_albedo.height = fallback_texture_height;

	fallback_normal.width = fallback_texture_width;
	fallback_normal.height = fallback_texture_height;

	fallback_orm.width = fallback_texture_width;
	fallback_orm.height = fallback_texture_height;

	// Create array to store pixels.

	if (const Result result = create_dyn_array(fallback_albedo.pixels, allocator, fallback_texture_size);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create dynamic array for fallback albedo texture\n");
		destroy_texture_storage();

		return result;
	}

	if (const Result result = create_dyn_array(fallback_normal.pixels, allocator, fallback_texture_size);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create dynamic array for fallback normal texture\n");
		destroy_texture_storage();
		destroy_dyn_array(fallback_albedo.pixels);

		return result;
	}

	if (const Result result = create_dyn_array(fallback_orm.pixels, allocator, fallback_texture_size);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to create dynamic array for fallback ORM texture\n");
		destroy_texture_storage();
		destroy_dyn_array(fallback_albedo.pixels);
		destroy_dyn_array(fallback_normal.pixels);

		return result;
	}

	// Generate pixel data for each fallback texture.
	// For the albedo texture, we generate recognizable checkboard pattern.

	// The size of each cell.
	constexpr uint32_t cell_size = fallback_texture_width / 8;

	for (uint32_t y = 0; y < fallback_texture_height; ++y)
	{
		// The row's cell index is constant across the row, so we compute it once instead of per pixel.
		const uint32_t cell_y = y / cell_size;

		for (uint32_t x = 0; x < fallback_texture_width; ++x)
		{
			push(fallback_normal.pixels, fallback_normal_color);
			push(fallback_orm.pixels, fallback_orm_color);

			const uint32_t cell_x = x / cell_size;

			// Stepping one cell along either axis flips the parity of the sum, so neighbouring cells alternate.
			if (const bool is_light_cell = (cell_x + cell_y) % 2 == 0)
			{
				push(fallback_albedo.pixels, fallback_albedo_light_color);
			}
			else
			{
				push(fallback_albedo.pixels, fallback_albedo_dark_color);
			}
		}
	}

	// Store this fallback textures.

	fallback_albedo_handle = load_texture(fallback_albedo);
	fallback_normal_handle = load_texture(fallback_normal);
	fallback_orm_handle = load_texture(fallback_orm);

	// Initialize fallback material.

	Material fallback_material = {};

	fallback_material.albedo = fallback_albedo_handle;
	fallback_material.normal = fallback_normal_handle;
	fallback_material.orm = fallback_orm_handle;

	// Store the fallback material.

	fallback_material_handle = load_material(fallback_material);

	return Result::SUCCESS;
}

void
blk::destroy_material_storage()
{
	destroy_resource_storage(storage);
}

blk::Pool_Handle<blk::Material>
blk::load_material(const char* stem)
{
	if (!BLK_VERIFY(stem))
	{
		return {};
	}

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

	Material material = {};

	material.albedo = load_texture(albedo_texture_hash);

	if (material.albedo == POOL_HANDLE_NONE<Texture>)
	{
		BLK_WARNING(
			"Failed to load albedo texture `%llu` of material `%s`; using fallback\n",
			albedo_texture_hash,
			stem
		);

		material.albedo = fallback_albedo_handle;
	}

	material.normal = load_texture(normal_texture_hash);

	if (material.normal == POOL_HANDLE_NONE<Texture>)
	{
		BLK_WARNING(
			"Failed to load normal texture `%llu` of material `%s`; using fallback\n",
			normal_texture_hash,
			stem
		);
		material.normal = fallback_normal_handle;
	}

	material.orm = load_texture(orm_texture_hash);

	if (material.orm == POOL_HANDLE_NONE<Texture>)
	{
		BLK_WARNING("Failed to load ORM texture `%llu` of material `%s`; using fallback\n", orm_texture_hash, stem);
		material.orm = fallback_orm_handle;
	}

	// We have to free `serial.buffer` because it is allocated by `load_resource`.
	free(*storage.pool.allocator, serial.buffer);

	return store_resource(storage, material, hash, stem);
}

blk::Pool_Handle<blk::Material>
blk::load_material(const Material& material)
{
	// We do not need hash nor stem when loading a material created at runtime.
	return store_resource(storage, material, 0, "");
}

void
blk::unload_material(Pool_Handle<Material> handle)
{
	release_resource(storage, handle);
}

blk::Material*
blk::get_material(Pool_Handle<Material> handle)
{
	Material* material = get_resource(storage, handle);

	if (!material)
	{
		return get_resource(storage, fallback_material_handle);
	}

	return material;
}
