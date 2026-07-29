#pragma once

#include "Core/Pool.hpp"
#include "Resource/Resource_Storage.hpp"

#include <stdint.h>

namespace blk
{
struct Shader
{
	Resource_Metadata metadata;
	char* buffer;
	uint32_t size;
};

Pool_Handle<Shader> load_shader(const char* stem);
void unload_shader(Pool_Handle<Shader> handle);
Shader* get_shader(Pool_Handle<Shader> handle);
}  // namespace blk
