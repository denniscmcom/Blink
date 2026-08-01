// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Shader.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

#include <stdlib.h>
#include <string>

namespace
{
blk::Resource_Storage<blk::Shader> shader_storage = {};
}  // namespace

blk::Pool_Handle<blk::Shader>
blk::load_shader(const char* stem)
{
	std::string path = "./Shaders/";
	path += stem;
	path += ".spv";

	if (is_resource_loaded(shader_storage, path.c_str()))
	{
		return get_resource_handle(shader_storage, path.c_str());
	}

	FILE* file = fopen(path.c_str(), "rb");

	if (!file)
	{
		BLK_FATAL("Failed to load shader");
	}

	fseek(file, 0, SEEK_END);
	const long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	Shader shader = {};
	shader.size = static_cast<uint32_t>(size);
	shader.buffer = static_cast<char*>(malloc(size));

	fread(shader.buffer, 1, size, file);
	fclose(file);

	return store_resource(shader_storage, shader, stem, path.c_str());
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
