// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <array>
#include <stdint.h>

namespace blk
{
using Magic = std::array<char, 4>;

constexpr Magic BLINK_MAGIC = {'B', 'L', 'N', 'K'};
constexpr size_t MAX_RESOURCE_LOGICAL_PATH_SIZE = 256;

enum class Resource_Type
{
	MATERIAL,
	MESH,
	SHADER,
	TEXTURE,
};

uint64_t get_resource_hash(const char* stem, Resource_Type resource_type);
}  // namespace blk
