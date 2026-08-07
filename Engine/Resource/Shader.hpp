// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Resource/Resource.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

#include <stdint.h>

namespace blk
{
struct Shader
{
	Resource_Metadata metadata;
	char* buffer;
	uint32_t size;
};

constexpr Magic SHADER_MAGIC = {'S', 'H', 'D', 'R'};
constexpr uint8_t SHADER_VERSION = 1;

Pool_Handle<Shader> load_shader(const char* stem);
void unload_shader(Pool_Handle<Shader> handle);
Shader* get_shader(Pool_Handle<Shader> handle);
}  // namespace blk
