// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Shader.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Resource.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

#include <format>
#include <stdlib.h>
#include <string>

namespace
{
blk::Resource_Storage<blk::Shader> shader_storage = {};
}  // namespace

blk::Pool_Handle<blk::Shader>
blk::load_shader(const char* stem)
{
	// TODO: This is duplicated mostly intact in the other `load_` functions.

	const uint64_t hash = get_resource_hash(stem, Resource_Type::SHADER);
	const std::string path = std::format("Shaders/{}.bshader", hash);

	if (is_resource_loaded(shader_storage, hash))
	{
		return get_resource_handle(shader_storage, hash);
	}

	File* file = open_file(path.c_str(), File_Access_Mode::READ);

	if (!file)
	{
		BLK_FATAL("Failed to load shader");
	}

	const uint64_t file_size = get_file_size(file);
	auto buffer = static_cast<char*>(malloc(file_size));
	read_file(file, buffer, file_size);
	close_file(file);

	Serial src_serial(buffer, file_size);

	if (src_serial.read<Magic>() != BLINK_MAGIC)
	{
		BLK_FATAL("Source buffer is not Blink format\n");
	}

	if (src_serial.read<Magic>() != SHADER_MAGIC)
	{
		BLK_FATAL("Source buffer is not a Blink shader\n");
	}

	if (src_serial.read<uint8_t>() != SHADER_VERSION)
	{
		BLK_FATAL("Expected Blink shader version %u\n", SHADER_VERSION);
	}

	Shader shader = {};
	// FIXME: Change `size` to uin64_t.
	shader.size = static_cast<uint32_t>(src_serial.size() - src_serial.position());
	shader.buffer = static_cast<char*>(malloc(shader.size));

	memcpy(shader.buffer, src_serial.buffer() + src_serial.position(), shader.size);

	return store_resource(shader_storage, shader, hash, stem);
}

void
blk::unload_shader(const Pool_Handle<Shader> handle)
{
	const Shader* shader = get_resource(shader_storage, handle);

	if (!shader)
	{
		return;
	}

	if (shader->buffer)
	{
		free(shader->buffer);
	}

	release_resource(shader_storage, handle);
}

blk::Shader*
blk::get_shader(const Pool_Handle<Shader> handle)
{
	return get_resource(shader_storage, handle);
}
