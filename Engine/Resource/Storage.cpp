// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Storage.hpp"

#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Resource/Material.hpp"
#include "Engine/Resource/Mesh.hpp"
#include "Engine/Resource/Shader.hpp"
#include "Engine/Resource/Texture.hpp"

#include <stdio.h>

uint64_t
blk::get_resource_hash(const char* stem, Resource_Type resource_type)
{
	// `filename` its just the resource `stem` plus the engine's custom extension of the compiled resource. For
	// example, a PNG texture in `Assets/Textures/Foo.png` gets compiled to `Assets/Foo.btexture`. Stem is `Foo` and
	// extension is `.btexture`, so the filename is `Foo.btexture`. This filename is what is used to compute the unique
	// resource hash.
	char filename[MAX_RESOURCE_FILENAME_SIZE];

	int written = 0;

	switch (resource_type)
	{
	case Resource_Type::MATERIAL:
		written = snprintf(filename, sizeof(filename), "%s.bmaterial", stem);
		break;
	case Resource_Type::MESH:
		written = snprintf(filename, sizeof(filename), "%s.bmesh", stem);
		break;
	case Resource_Type::SHADER:
		written = snprintf(filename, sizeof(filename), "%s.bshader", stem);
		break;
	case Resource_Type::TEXTURE:
		written = snprintf(filename, sizeof(filename), "%s.btexture", stem);
		break;
	}

	if (!BLK_VERIFY(written > 0 && static_cast<size_t>(written) < MAX_RESOURCE_FILENAME_SIZE))
	{
		return 0;
	}

	return hash_fnv1a(filename);
}

size_t
blk::get_resource_logical_path(uint64_t hash, Resource_Type resource_type, char* logical_path)
{
	if (!BLK_VERIFY(logical_path))
	{
		return 0;
	}

	int written = 0;

	switch (resource_type)
	{
	case Resource_Type::MATERIAL:
		written = snprintf(logical_path, MAX_RESOURCE_LOGICAL_PATH_SIZE, "Assets/%llu.bmaterial", hash);
		break;
	case Resource_Type::MESH:
		written = snprintf(logical_path, MAX_RESOURCE_LOGICAL_PATH_SIZE, "Assets/%llu.bmesh", hash);
		break;
	case Resource_Type::SHADER:
		written = snprintf(logical_path, MAX_RESOURCE_LOGICAL_PATH_SIZE, "Assets/%llu.bshader", hash);
		break;
	case Resource_Type::TEXTURE:
		written = snprintf(logical_path, MAX_RESOURCE_LOGICAL_PATH_SIZE, "Assets/%llu.btexture", hash);
		break;
	}

	if (!BLK_VERIFY(written > 0 && static_cast<size_t>(written) < MAX_RESOURCE_LOGICAL_PATH_SIZE))
	{
		return 0;
	}

	return static_cast<size_t>(written);
}

blk::Result
blk::deserialize_resource(Allocator* allocator, Resource_Type type, uint64_t hash, Serial& serial)
{
	if (!BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	// Gets the resource logical path.
	char logical_path[MAX_RESOURCE_LOGICAL_PATH_SIZE];

	if (get_resource_logical_path(hash, type, logical_path) == 0)
	{
		BLK_ERROR("Failed to get the logical path of resource `%llu`\n", hash);

		return Result::INVALID_ARGUMENTS;
	}

	// Now, load the resource data into a `Serial` to deserialize it.

	serial = {};

	// Open file.
	File* file = nullptr;

	if (const Result result = open_file(allocator, logical_path, File_Access_Mode::READ, file);
		result != Result::SUCCESS)
	{
		BLK_ERROR("Failed to open file `%s`\n", logical_path);

		return result;
	}

	// Get file size.
	size_t size = 0;

	if (const Result result = get_file_size(file, size); result != Result::SUCCESS)
	{
		BLK_VERIFY(close_file(file) == Result::SUCCESS);

		return result;
	}

	// Allocate a buffer for the file data.
	void* pointer = nullptr;

	if (const Result result = allocate(*allocator, pointer, size, alignof(char)); result != Result::SUCCESS)
	{
		BLK_VERIFY(close_file(file) == Result::SUCCESS);

		return result;
	}

	// Initialize `Serial`.
	serial = init_serial(static_cast<char*>(pointer), size);

	// Copy the file data to `serial.buffer`.
	if (const Result result = read_file(file, serial.buffer, serial.size); result != Result::SUCCESS)
	{
		BLK_VERIFY(close_file(file) == Result::SUCCESS);
		free(*allocator, serial.buffer);
		serial = {};

		return result;
	}

	// We can now close the file.
	BLK_VERIFY(close_file(file) == Result::SUCCESS);

	// Check if the file is a valid engine asset.
	char file_magic[4];

	BLK_IF_NOT_SUCCESS(read(serial, file_magic))
	{
		free(*allocator, serial.buffer);
		serial = {};

		return Result::INVALID_FILE_FORMAT;
	}

	if (memcmp(file_magic, BLINK_MAGIC, 4) != 0)
	{
		free(*allocator, serial.buffer);
		serial = {};

		return Result::INVALID_FILE_FORMAT;
	}

	// Check if the file is a valid resource engine asset.
	char resource_magic[4];

	BLK_IF_NOT_SUCCESS(read(serial, resource_magic))
	{
		free(*allocator, serial.buffer);
		serial = {};

		return Result::INVALID_FILE_FORMAT;
	}

	// Check if the file version is compatible.
	uint8_t resource_version = 0;

	BLK_IF_NOT_SUCCESS(read(serial, resource_version))
	{
		free(*allocator, serial.buffer);
		serial = {};

		return Result::INVALID_FILE_FORMAT;
	}

	int resource_magic_result = 0;
	bool is_resource_compatible = false;

	switch (type)
	{
	case Resource_Type::MATERIAL:
		resource_magic_result = memcmp(resource_magic, MATERIAL_MAGIC, 4);
		is_resource_compatible = resource_version == MATERIAL_VERSION;
		break;
	case Resource_Type::MESH:
		resource_magic_result = memcmp(resource_magic, MESH_MAGIC, 4);
		is_resource_compatible = resource_version == MESH_VERSION;
		break;
	case Resource_Type::SHADER:
		resource_magic_result = memcmp(resource_magic, SHADER_MAGIC, 4);
		is_resource_compatible = resource_version == SHADER_VERSION;
		break;
	case Resource_Type::TEXTURE:
		resource_magic_result = memcmp(resource_magic, TEXTURE_MAGIC, 4);
		is_resource_compatible = resource_version == TEXTURE_VERSION;
		break;
	}

	if (resource_magic_result != 0 || !is_resource_compatible)
	{
		free(*allocator, serial.buffer);
		serial = {};

		return Result::INVALID_FILE_FORMAT;
	}

	// Caller should deallocate `serial.buffer`.

	return Result::SUCCESS;
}
