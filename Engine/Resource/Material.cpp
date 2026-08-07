// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Material.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Resource/Resource.hpp"
#include "Engine/Resource/Resource_Storage.hpp"
#include "Engine/Resource/Texture.hpp"

#include <format>
#include <string>

namespace
{
blk::Resource_Storage<blk::Material> material_storage = {};
}  // namespace

blk::Pool_Handle<blk::Material>
blk::load_material(const char* stem)
{
	const uint64_t hash = get_resource_hash(stem, Resource_Type::MATERIAL);
	const std::string path = std::format("Assets/{}.bmaterial", hash);

	if (is_resource_loaded(material_storage, hash))
	{
		return get_resource_handle(material_storage, hash);
	}

	File* file = open_file(path.c_str(), File_Access_Mode::READ);

	if (!file)
	{
		BLK_FATAL("Failed to open asset: %s\n", path.c_str());
	}

	const uint64_t file_size = get_file_size(file);
	auto buffer = static_cast<char*>(malloc(file_size));
	read_file(file, buffer, file_size);
	close_file(file);

	Serial serial(buffer, file_size);

	if (serial.read<Magic>() != BLINK_MAGIC)
	{
		BLK_FATAL("Source buffer is not Blink format\n");
	}

	if (serial.read<Magic>() != MATERIAL_MAGIC)
	{
		BLK_FATAL("Source buffer is not a Blink material\n");
	}

	if (serial.read<uint8_t>() != MATERIAL_VERSION)
	{
		BLK_FATAL("Expected Blink mesh version %u\n", MATERIAL_VERSION);
	}

	Material material = {};
	material.diffuse_map = load_texture(serial.read<uint64_t>());
	material.specular_map = load_texture(serial.read<uint64_t>());
	material.shininess = serial.read<float>();

	return store_resource(material_storage, material, hash, stem);
}

void
blk::unload_material(Pool_Handle<Material> handle)
{
}

blk::Material*
blk::get_material(Pool_Handle<Material> handle)
{
	return get_resource(material_storage, handle);
}
