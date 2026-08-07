// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Core/Hash.hpp"

#include <stdlib.h>
#include <string.h>

namespace
{
constexpr uint64_t FNV_OFFSET_BASIS = 0xCBF29CE484222325;
constexpr uint64_t FNV_PRIME = 0x100000001B3;
}  // namespace

uint64_t
blk::hash_fnv1a(const char* buffer, const size_t size)
{
	uint64_t hash = FNV_OFFSET_BASIS;

	for (size_t i = 0; i < size; i++)
	{
		// FIXME: Should I cast `buffer[i]` to unsigned char first because MSVC?
		hash ^= static_cast<uint64_t>(buffer[i]);
		hash *= FNV_PRIME;
	}

	return hash;
}

uint64_t
blk::hash_fnv1a(const char* string)
{
	return hash_fnv1a(string, strlen(string));
}
