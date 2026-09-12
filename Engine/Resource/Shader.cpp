// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Shader.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Storage.hpp"

namespace
{
blk::Resource_Storage<blk::Shader> storage = {};
}  // namespace

blk::Result
blk::create_shader_storage(Allocator* allocator)
{
	return create_resource_storage(storage, allocator);
}

void
blk::destroy_shader_storage()
{
	// TODO (Bug): We are leaking `Shader::buffer`.
	// It is allocated by `load_shader`, so we should free it before destroying `storage`.
	// I have not fixed this yet because it is not really that important. We only destroy `storage` on application exit,
	// so in reality we are not leaking anything.
	destroy_resource_storage(storage);
}

blk::Pool_Handle<blk::Shader>
blk::load_shader(const char* stem)
{
	if (!BLK_VERIFY(stem))
	{
		return {};
	}

	// Compute the shader hash.
	const uint64_t hash = get_resource_hash(stem, Resource_Type::SHADER);

	if (is_resource_loaded(storage, hash))
	{
		// If the shader is already loaded, we return its handle.
		return get_resource_handle(storage, hash);
	}

	Serial serial = {};

	// `deserialize_resource` allocates `serial.buffer` and handle ownership to us, so we need to free it.
	BLK_IF_NOT_SUCCESS(deserialize_resource(storage.pool.allocator, Resource_Type::SHADER, hash, serial))
	{
		BLK_ERROR("Failed to deserialize shader `%llu`\n", hash);

		return {};
	}

	// Initialize a `Shader`.
	Shader shader = {};

	// We can compute the size of `buffer` by subtracting the current `serial.position` from the total `serial.size`
	// because there are no more data after.
	shader.size = static_cast<uint32_t>(serial.size - serial.position);

	// TODO (Bug): This alignment is a graphics API concern leaking into `Resource/`. `buffer` holds SPIR-V words and
	// Vulkan requires `VkShaderModuleCreateInfo::pCode` to be 4-byte aligned, so aligning it to `char` gives an address
	// Vulkan may reject. It is not important right now because Vulkan is the only backend, but `Resource/` should not
	// know about it.

	// Now, we allocate the buffer using `shader.size`.
	void* pointer = nullptr;

	BLK_IF_NOT_SUCCESS(allocate(*storage.pool.allocator, pointer, shader.size, alignof(uint32_t)))
	{
		BLK_ERROR("Failed to allocate shader source buffer\n");
		free(*storage.pool.allocator, serial.buffer);

		return {};
	}

	shader.buffer = static_cast<char*>(pointer);

	// Finally, we just copy the rest of the contents in `serial` to `shader.buffer`.
	memcpy(shader.buffer, serial.buffer + serial.position, shader.size);

	// We have to free `serial.buffer` because it is allocated by `load_resource`.
	free(*storage.pool.allocator, serial.buffer);

	return store_resource(storage, shader, hash, stem);
}

void
blk::unload_shader(const Pool_Handle<Shader> handle)
{
	// First, we have to free `shader.buffer` because it was allocated by `load_shader`.
	const Shader* shader = get_resource(storage, handle);

	if (!shader)
	{
		// If `shader` does not exists, there is nothing to unload.
		return;
	}

	// This should always be true if `shader` was correctly loaded.
	if (shader->buffer)
	{
		free(*storage.pool.allocator, shader->buffer);
	}

	// Finally, we release the shader from `storage`.
	release_resource(storage, handle);
}

blk::Shader*
blk::get_shader(const Pool_Handle<Shader> handle)
{
	return get_resource(storage, handle);
}
