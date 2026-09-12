// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Core/Hash.hpp"

#include <stdint.h>
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
		// Without casting to `unsigned char` first, `0xFF` could extend to `0xFFFFFFFFFFFFFFFF` instead of
		// `0x00000000000000FF`, corrupting the hash.
		hash ^= static_cast<uint64_t>(static_cast<unsigned char>(buffer[i]));
		hash *= FNV_PRIME;
	}

	return hash;
}

uint64_t
blk::hash_fnv1a(const char* string)
{
	return hash_fnv1a(string, strlen(string));
}

uint64_t
blk::hash_fnv1a(uint64_t value)
{
	// TODO (Bug): The resulting hash will be endianness-dependent.
	// The same integer will produce different hashes on little-endian vs big-endian machines. This is bad for
	// cross-platform determinism (e.g. serialized asset IDs).
	return hash_fnv1a(reinterpret_cast<const char*>(&value), sizeof(value));
}
