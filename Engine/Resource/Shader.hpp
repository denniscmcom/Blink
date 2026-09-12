// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Resource/Storage.hpp"

#include <stdint.h>

namespace blk
{
/// A deserialized shader.
struct Shader
{
	/// Shader's metadata.
	Resource_Metadata metadata;
	/// Shader's source code.
	char* buffer;
	/// Size of `buffer`.
	uint32_t size;
};

/// Blink's shader magic number. It is found after `BLINK_MAGIC` in `.bshader` binary files.
constexpr char SHADER_MAGIC[4] = {'S', 'H', 'D', 'R'};
/// The current implementation version of `.bshader` files.
constexpr uint8_t SHADER_VERSION = 1;

/// Creates the storage for shaders.
Result create_shader_storage(Allocator* allocator);
/// Destroys the storage for shaders.
void destroy_shader_storage();
/// Loads a `.bshader` file into memory.
Pool_Handle<Shader> load_shader(const char* stem);
/// Unloads a shader from memory.
void unload_shader(Pool_Handle<Shader> handle);
/// Gets a pointer to a shader's data.
Shader* get_shader(Pool_Handle<Shader> handle);
}  // namespace blk
